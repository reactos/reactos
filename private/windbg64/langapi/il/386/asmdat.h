/*static char *SCCSID = "@(#)asmdat.h:1.12";*/

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

/***	asmdat.h -- ASM information
 * SYNOPSIS
 *		... stuff ...
 *		#define	DAT( id, str, ...  )	... stuff ...
 *		#include "asmdat.h"
 *		#undef	DAT
 *		... stuff ...
 * DESCRIPTION
 *		This file is included in a number of different contexts to
 *		generate the asm opcode enum and various tables.
 *
 *		QDAT set of opcodes common to inline assembler and Hi C and QC
 *		CDAT set of opcodes common to inline assembler and Hi C
 *		IDAT set of opcodes only required by inline assembler
 *		    this assumes QC opcodes are a subset of Hi C opcodes,
 *		    and the set of QC and Hi C opcodes are a subset of
 *		    inline assembler.
 *		QPDAT opcodes are QC-only pseudo-ops
 *		CPDAT opcodes are cmerge-only pseudo-ops
 *		    these sets are mutually exclusive.  A pseudo op that is
 *		    required for both QC and Hi C must appear twice.
 *		    (eg, DATA and PDATA)
 *		TPDAT opcodes are defined for 2-pass QC which requires
 *			all opcodes.
 *
 *		Synonymous instructions have only one representation.
 *		the names of pseudo ops begin with _ and end with a space,
 *		so they appear in the table, but will never match a
 *		mnemonic read by P1.
 *		Why the underscore?  That keeps them out of the way when the
 *		table is made for mnemonic lookup.
 *		This allows the macros to be independent of flavor.
 *		Note that IDAT with an opcode that is not an identifer, does not
 *		make sense.
 *		cmerge CBD: The opcode enums are prefixed for p1 and QC,
 *		but not for p2/p3.  This makes some features inaccessible.
 *		QC was almost a subset of cmerge, except for these:
 *		faddp, fsubp, fdivp, fmulp.
 *		now these are QDAT instead of IDAT - though cmerge without
 *		inline assembler doesn't use them.
 *		881013: OPCCLs no longer used.  That field could be removed from
 *		all these DAT "records" if we could get everyone (P1, P2, P3, QC)
 *		to change their DAT macro at the same time.  For now, we will just
 *		leave it in and ignore it.
 */
#ifndef	ASM_ONIL
#define	ASM_ONIL	0xff
#endif

#define	XX		0		/* M00BUG todo */
#define	ACLS_NIL	-1
#define	ASM_MNIL	-1

#define	QDAT		DAT		/* QC and cmerge */
#define	CDAT		DAT		/* cmerge only */
#define	IDAT		DAT		/* inline asm only */
#define	QPDAT		DAT		/* QC only pseudo op */
#define	CPDAT		DAT		/* cmerge only pseudo op */
#define	TPDAT		DAT		/* QC 2-Pass pseudo op */

#define	ZDAT( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)	/* NOTHING */


#if CC_QC2PASS /* { include ALL asm opcodes for now */
#undef	CPDAT
#define	CPDAT	ZDAT
#undef	QPDAT
#define	QPDAT	ZDAT

#else	/* !CC_QC2PASS */
#undef	QPDAT
#define QPDAT	ZDAT
#undef	TPDAT
#define	TPDAT	ZDAT

#endif /* } */

#if ! CC_ASM
#undef	IDAT
#define	IDAT	ZDAT
#endif

/* opcodes GROUP { */
// M00MERGE: ASM_OOPN bug in two-pass?
#define ASM_OOP0	ASM_OFIRSTOP

// M00MERGE: TPDAT() is a move in the wrong direction.
// M00MERGE: each op should only be here once...
// TRUE, but we have to adjust QC - pass 2 to know about CMERGE pass 1 opcodes.
//	Jan
/*   	op,			dope,			num_oprs,	osize,	byte2					name,	ACLS,		ASM,		template,				XX,		machine */
CPDAT(	FIRSTOP,	0,					0,		0,		0,						"_firstop ",ACLS_NIL,	ASM_MNIL,0,						XX,		P_86 )
TPDAT(	FIRSTOP,	0,					0,		0,		0,						"_firstop ",ACLS_NIL,	ASM_MNIL,0,						XX,		P_86 )
QDAT(	MOV,		000000|000000,		2,		1,		R000,					"mov",		ACLS_MOVE,	ASM_M0,	"\210\306\260\240\242\216\214",XX,	P_86 )
IDAT(	ARPL,		000000|000000,		2,		1,		RNON,					"arpl",		ACLS_ARPL,	ASM_M0,	"\143",					XX,		P_PROT|P_286 )
CDAT(	BOUND,		000000|000000,		2,		1,		RNON,					"bound",	ACLS_BINOP,	ASM_M3,	"\142",					XX,		P_186 )
QDAT(	ENTER,		000000|000000,		2,		4,		RNON,					"enter",	ACLS_ENTER,	ASM_M0,	"\310",					XX,		P_186 )
CDAT(	ESC,		000000|000000,		2,		1,		RNON,					"esc",		ACLS_ESC,	ASM_M0,	"\330",					XX,		P_ONLY86 | P_ONLY186 | P_ONLY286)
QDAT(	IMUL,		000000|DSETCC,		1,		1,		R101,					"imul",		ACLS_UNARY,	ASM_M5,	"\366\151",				XX,		P_86 )
CDAT(	INTR,		DUSECC|DSETCC,		1,		1,		RNON,					"int",		ACLS_INT,	ASM_M0,	"\315\314",				XX,		P_86 )
QDAT(	RET,		000000|DSETCC,		1,		1,		RNON,					"ret",		ACLS_RETC,	ASM_M0,	"\303\302\313\312",		XX,		P_86 )
QDAT(	XCHG,		000000|000000,		2,		1,		R000,					"xchg",		ACLS_XCHG,	ASM_M0,	"\206\220",				XX,		P_86 )
CDAT(	IN,			000000|000000,		2,		1,		RNON,					"in",		ACLS_IO,	ASM_M0,	"\344\354\345\355",		XX,		P_86 )
CDAT(	OUT,		000000|000000,		2,		1,		RNON,					"out",		ACLS_IO,	ASM_M1,	"\346\356\347\357",		XX,		P_86 )
QDAT(	POP,		000000|000000,		1,		1,		R000,					"pop",		ACLS_POP,	ASM_M0,	"\217\130\7",			XX,		P_86 )
QDAT(	PUSH,		000000|000000,		1,		1,		R110,					"push",		ACLS_PUSH,	ASM_M0,	"\377\120\6\150",		XX,		P_86 )
QDAT(	CALL,		000000|000000,		1,		1,		RNON,					"call",		ACLS_CALL,	ASM_M0,	"\350\377\232\377",		XX,		P_86 )
QDAT(	JCC,		DUSECC|000000,		1,		2,		RNON,					"_jcc ",	ACLS_JCC,	ASM_M0,	"\160\200",				XX,		P_86 ) /*	Lowest	JCC	value.	*/
QDAT(	JMP,		000000|000000,		1,		1,		RNON,					"jmp",		ACLS_JMP,	ASM_M0,	"\353\351\377\352\377",	XX,		P_86 )
QDAT(	LDS,		000000|000000,		2,		1,		RNON,					"lds",		ACLS_BINOP,	ASM_M1,	"\305",					XX,		P_86 )
QDAT(	LEA,		000000|000000,		2,		1,		RNON,					"lea",		ACLS_BINOP,	ASM_M0,	"\215",					XX,		P_86 )
QDAT(	LES,		000000|000000,		2,		1,		RNON,					"les",		ACLS_BINOP,	ASM_M2,	"\304",					XX,		P_86 )
IDAT(	LAR,		DZEROF|000000,		2,		2,		RNON,					"lar",		ACLS_X2B,	ASM_M0,	"\2",					XX,		P_PROT|P_286 )
IDAT(	LSL,		DZEROF|000000,		2,		2,		RNON,					"lsl",		ACLS_X2B,	ASM_M1,	"\3",					XX,		P_PROT|P_286 )
CDAT(	JCXZ,		000000|000000,		1,		2,		RNON,					"jcxz",		ACLS_LOOP,	ASM_M3,	"\343",					XX,		P_86 )
CDAT(	LOOP,		000000|000000,		1,		2,		RNON,					"loop",		ACLS_LOOP,	ASM_M2,	"\342",					XX,		P_86 )
CDAT(	LOOPNZ,		DUSECC|000000,		1,		2,		RNON,					"loopnz",	ACLS_LOOP,	ASM_M0,	"\340",					XX,		P_86 )
CDAT(	LOOPZ,		DUSECC|000000,		1,		2,		RNON,					"loopz",	ACLS_LOOP,	ASM_M1,	"\341",					XX,		P_86 )
IDAT(	LGDT,		DZEROF|000000,		1,		2,		R010,					"lgdt",		ACLS_X11,	ASM_M2,	"\1",					XX,		P_PROT|P_286 )
IDAT(	LIDT,		DZEROF|000000,		1,		2,		R011,					"lidt",		ACLS_X11,	ASM_M3,	"\1",					XX,		P_PROT|P_286 )
IDAT(	SGDT,		DZEROF|000000,		1,		2,		R000,					"sgdt",		ACLS_X11,	ASM_M0,	"\1",					XX,		P_PROT|P_286 )
IDAT(	SIDT,		DZEROF|000000,		1,		2,		R001,					"sidt",		ACLS_X11,	ASM_M1,	"\1",					XX,		P_PROT|P_286 )
QDAT(	DEC,		000000|DSETCC,		2,		1,		R001,					"dec",		ACLS_INC,	ASM_M1,	"\376\110",				XX,		P_86 )
QDAT(	DIV,		000000|DSETCC,		1,		1,		R110,					"div",		ACLS_UNARY,	ASM_M6,	"\366",					XX,		P_86 )
QDAT(	IDIV,		000000|DSETCC,		1,		1,		R111,					"idiv",		ACLS_UNARY,	ASM_M7,	"\366",					XX,		P_86 )
QDAT(	INC,		000000|DSETCC,		2,		1,		R000,					"inc",		ACLS_INC,	ASM_M0,	"\376\100",				XX,		P_86 )
QDAT(	MUL,		000000|DSETCC,		1,		1,		R100,					"mul",		ACLS_UNARY,	ASM_M4,	"\366",					XX,		P_86 )
QDAT(	NEG,		000000|DSETCC,		1,		1,		R011,					"neg",		ACLS_UNARY,	ASM_M3,	"\366",					XX,		P_86 )
QDAT(	NOT,		000000|000000,		1,		1,		R010,					"not",		ACLS_UNARY,	ASM_M2,	"\366",					XX,		P_86 )
QDAT(	RCL,		DUSECC|DSETCC,		2,		1,		R010,					"rcl",		ACLS_SHIFT,	ASM_M2,	"\320\322\300",			XX,		P_86 )
QDAT(	RCR,		DUSECC|DSETCC,		2,		1,		R011,					"rcr",		ACLS_SHIFT,	ASM_M3,	"\320\322\300",			XX,		P_86 )
QDAT(	ROL,		000000|DSETCC,		2,		1,		R000,					"rol",		ACLS_SHIFT,	ASM_M0,	"\320\322\300",			XX,		P_86 )
QDAT(	ROR,		000000|DSETCC,		2,		1,		R001,					"ror",		ACLS_SHIFT,	ASM_M1,	"\320\322\300",			XX,		P_86 )
QDAT(	SAR,		000000|DSETCC,		2,		1,		R111,					"sar",		ACLS_SHIFT,	ASM_M7,	"\320\322\300",			XX,		P_86 )
QDAT(	SHL,		DUSECC|DSETCC,		2,		1,		R100,					"shl",		ACLS_SHIFT,	ASM_M4,	"\320\322\300",			XX,		P_86 )
QDAT(	SHR,		000000|DSETCC,		2,		1,		R101,					"shr",		ACLS_SHIFT,	ASM_M5,	"\320\322\300",			XX,		P_86 )
QDAT(	ADC,		DUSECC|DSETCC,		2,		1,		R010,					"adc",		ACLS_DIROP,	ASM_M2,	"\20\200\24",			XX,		P_86 )
QDAT(	ADD,		000000|DSETCC,		2,		1,		R000,					"add",		ACLS_DIROP,	ASM_M0,	"\0\200\4",				XX,		P_86 )
QDAT(	AND,		000000|DSETCC,		2,		1,		R100,					"and",		ACLS_DIROP,	ASM_M4,	"\40\200\44",			XX,		P_86 )
QDAT(	CMP,		000000|DSETCC,		2,		1,		R111,					"cmp",		ACLS_DIROP,	ASM_M7,	"\70\200\74",			XX,		P_86 )
QDAT(	OR,			000000|DSETCC,		2,		1,		R001,					"or",		ACLS_DIROP,	ASM_M1,	"\10\200\14",			XX,		P_86 )
QDAT(	SBB,		DUSECC|DSETCC,		2,		1,		R011,					"sbb",		ACLS_DIROP,	ASM_M3,	"\30\200\34",			XX,		P_86 )
QDAT(	SUB,		000000|DSETCC,		2,		1,		R101,					"sub",		ACLS_DIROP,	ASM_M5,	"\50\200\54",			XX,		P_86 )
QDAT(	TEST,		000000|DSETCC,		2,		1,		R000,					"test",		ACLS_TEST,	ASM_M0,	"\204\366\250",			XX,		P_86 )
QDAT(	XOR,		000000|DSETCC,		2,		1,		R110,					"xor",		ACLS_DIROP,	ASM_M6,	"\60\200\64",			XX,		P_86 )
QDAT(	CMPS,		000000|DSETCC,		0,		1,		RNON,					"cmps",		ACLS_STR,	ASM_M1,	"\246",					XX,		P_86 )
CDAT(	INS,		000000|000000,		0,		1,		RNON,					"ins",		ACLS_STR,	ASM_M5,	"\154",					XX,		P_186 )
QDAT(	LODS,		000000|000000,		0,		1,		RNON,					"lods",		ACLS_STR,	ASM_M3,	"\254",					XX,		P_86 )
QDAT(	MOVS,		000000|000000,		0,		1,		RNON,					"movs",		ACLS_STR,	ASM_M0,	"\244",					XX,		P_86 )
CDAT(	OUTS,		000000|000000,		0,		1,		RNON,					"outs",		ACLS_STR,	ASM_M6,	"\156",					XX,		P_186 )
QDAT(	SCAS,		DUSECC|DSETCC,		0,		1,		RNON,					"scas",		ACLS_STR,	ASM_M4,	"\256",					XX,		P_86 )
QDAT(	STOS,		000000|000000,		0,		1,		RNON,					"stos",		ACLS_STR,	ASM_M2,	"\252",					XX,		P_86 )
CDAT(	XLAT,		000000|000000,		0,		1,		RNON,					"xlat",		ACLS_IMM3,	ASM_M0,	"\327",					XX,		P_86 )
IDAT(	LLDT,		DZEROF|000000,		1,		2,		R010,					"lldt",		ACLS_X10,	ASM_M2,	"\0",					XX,		P_PROT|P_286 )
IDAT(	LMSW,		DZEROF|000000,		1,		2,		R110,					"lmsw",		ACLS_X11,	ASM_M6,	"\1",					XX,		P_PROT|P_286 )
IDAT(	LTR,		DZEROF|000000,		1,		2,		R011,					"ltr",		ACLS_X10,	ASM_M3,	"\0",					XX,		P_PROT|P_286 )
IDAT(	SLDT,		DZEROF|000000,		1,		2,		R000,					"sldt",		ACLS_X10,	ASM_M0,	"\0",					XX,		P_PROT|P_286 )
IDAT(	SMSW,		DZEROF|000000,		1,		2,		R100,					"smsw",		ACLS_X11,	ASM_M4,	"\1",					XX,		P_PROT|P_286 )
IDAT(	STR,		DZEROF|000000,		1,		2,		R001,					"str",		ACLS_X10,	ASM_M1,	"\0",					XX,		P_PROT|P_286 )
IDAT(	VERR,		DZEROF|000000,		1,		2,		R100,					"verr",		ACLS_X10,	ASM_M4,	"\0",					XX,		P_PROT|P_286 )
IDAT(	VERW,		DZEROF|000000,		1,		2,		R101,					"verw",		ACLS_X10,	ASM_M5,	"\0",					XX,		P_PROT|P_286 )
QDAT(	FADD,		DFLT|00000000,		2,		2,		R000,					"fadd",		ACLS_FOP,	ASM_M0,	"\0",					XX,		P_86 )
QDAT(	FMUL,		DFLT|00000000,		2,		2,		R001,					"fmul",		ACLS_FOP,	ASM_M1,	"\0",					XX,		P_86 )
QDAT(	FSUB,		DFLT|00000000,		2,		2,		R100,					"fsub",		ACLS_FOP,	ASM_M4,	"\0",					XX,		P_86 )
QDAT(	FSUBR,		DFLT|00000000,		2,		2,		R101,					"fsubr",	ACLS_FOP,	ASM_M5,	"\0",					XX,		P_86 )
QDAT(	FDIV,		DFLT|00000000,		2,		2,		R110,					"fdiv",		ACLS_FOP,	ASM_M6,	"\0",					XX,		P_86 )
QDAT(	FDIVR,		DFLT|00000000,		2,		2,		R111,					"fdivr",	ACLS_FOP,	ASM_M7,	"\0",					XX,		P_86 )
QDAT(	FADDP,		DFLT|00000000,		2,		3,		0xc1,					"faddp",	ACLS_FOPP,	ASM_M0,	"\6",					XX,		P_86 )
QDAT(	FMULP,		DFLT|00000000,		2,		3,		0xc9,					"fmulp",	ACLS_FOPP,	ASM_M1,	"\6",					XX,		P_86 )
QDAT(	FSUBP,		DFLT|00000000,		2,		3,		0xe9,					"fsubp",	ACLS_FOPP,	ASM_M4,	"\6",					XX,		P_86 )
IDAT(	FSUBRP,		DFLT|00000000,		2,		2,		R100,					"fsubrp",	ACLS_FOPP,	ASM_M5,	"\0",					XX,		P_86 )
QDAT(	FDIVP,		DFLT|00000000,		2,		3,		0xf9,					"fdivp",	ACLS_FOPP,	ASM_M6,	"\6",					XX,		P_86 )
IDAT(	FDIVRP,		DFLT|00000000,		2,		2,		R111,					"fdivrp",	ACLS_FOPP,	ASM_M7,	"\0",					XX,		P_86 )
CDAT(	FIADD,		DFLT|00000000,		2,		2,		R000,					"fiadd",	ACLS_FIOP,	ASM_M0,	"\0",					XX,		P_86 )
CDAT(	FICOM,		DFLT|00000000,		2,		2,		R010,					"ficom",	ACLS_FIOP,	ASM_M2,	"\0",					XX,		P_86 )
CDAT(	FICOMP,		DFLT|00000000,		2,		2,		R011,					"ficomp",	ACLS_FIOP,	ASM_M3,	"\0",					XX,		P_86 )
CDAT(	FIDIV,		DFLT|00000000,		2,		2,		R110,					"fidiv",	ACLS_FIOP,	ASM_M6,	"\0",					XX,		P_86 )
CDAT(	FIDIVR,		DFLT|00000000,		2,		2,		R111,					"fidivr",	ACLS_FIOP,	ASM_M7,	"\0",					XX,		P_86 )
CDAT(	FILD,		DFLT|00000000,		2,		2,		R000,					"fild",		ACLS_FIMOVE,	ASM_M0,	"\1",				XX,		P_86 )
CDAT(	FIMUL,		DFLT|00000000,		2,		2,		R001,					"fimul",	ACLS_FIOP,	ASM_M1,	"\0",					XX,		P_86 )
CDAT(	FISUB,		DFLT|00000000,		2,		2,		R100,					"fisub",	ACLS_FIOP,	ASM_M4,	"\0",					XX,		P_86 )
CDAT(	FISUBR,		DFLT|00000000,		2,		2,		R101,					"fisubr",	ACLS_FIOP,	ASM_M5,	"\0",					XX,		P_86 )
CDAT(	FIST,		DFLT|00000000,		1,		2,		R010,					"fist",		ACLS_FIMOVE,	ASM_M2,	"\1",				XX,		P_86 )
CDAT(	FISTP,		DFLT|00000000,		1,		2,		R011,					"fistp",	ACLS_FIMOVE,	ASM_M3,	"\1",				XX,		P_86 )
QDAT(	FCOM,		DFLT|00000000,		2,		2,		R010,					"fcom",		ACLS_FOP,	ASM_M2,	"\0",					XX,		P_86 )
QDAT(	FCOMP,		DFLT|00000000,		2,		2,		R011,					"fcomp",	ACLS_FOP,	ASM_M3,	"\0",					XX,		P_86 )
CDAT(	FCOMPP,		DFLT|00000000,		2,		3,		0xd9,					"fcompp",	ACLS_FIMMW,	ASM_M1,	"\6",					XX,		P_86 )
QDAT(	FXCH,		DFLT|00000000,		1,		2,		R001,					"fxch",		ACLS_FUOP,	ASM_M1,	"\1",					XX,		P_86 )
QDAT(	FLD,		DFLT|00000000,		1,		2,		R000,					"fld",		ACLS_FMOVE,	ASM_M0,	"\1",					XX,		P_86 )
CDAT(	FFREE,		DFLT|00000000,		1,		2,		R000,					"ffree",	ACLS_FUOP,	ASM_M0,	"\5",					XX,		P_86 )
QDAT(	FST,		DFLT|00000000,		1,		2,		R010,					"fst",		ACLS_FMOVE,	ASM_M2,	"\5",					XX,		P_86 )
QDAT(	FSTP,		DFLT|00000000,		1,		2,		R011,					"fstp",		ACLS_FMOVE,	ASM_M3,	"\5",					XX,		P_86 )
IDAT(	FBLD,		DFLT|00000000,		1,		2,		R100,					"fbld",		ACLS_FBMOVE,	ASM_M0,	"\7",				XX,		P_86 )
IDAT(	FLDCW,		DFLT|00000000,		1,		2,		R101,					"fldcw",	ACLS_FCNTL,	ASM_M4,	"\1",					XX,		P_86 )
IDAT(	FLDENV,		DFLT|00000000,		1,		2,		R100,					"fldenv",	ACLS_FCNTL,	ASM_M5,	"\1",					XX,		P_86 )
IDAT(	FRSTOR,		DFLT|00000000,		1,		2,		R100,					"frstor",	ACLS_FCNTL,	ASM_M6,	"\5",					XX,		P_86 )
IDAT(	FSAVE,		DFLT|00000000,		1,		2,		R110,					"fsave",	ACLS_FCNTL,	ASM_M2,	"\5",					XX,		P_86 )
IDAT(	FBSTP,		DFLT|00000000,		1,		2,		R110,					"fbstp",	ACLS_FBMOVE,	ASM_M3,	"\7",				XX,		P_86 )
IDAT(	FSTCW,		DFLT|DNOMF,			1,		2,		R111,					"fstcw",	ACLS_FCNTL,	ASM_M0,	"\1",					XX,		P_86 )
IDAT(	FSTENV,		DFLT|00000000,		1,		2,		R110,					"fstenv",	ACLS_FCNTL,	ASM_M3,	"\1",					XX,		P_86 )
QDAT(	FSTSW,		DFLT|DNOMF,			1,		2,		R111,					"fstsw",	ACLS_FCNTL,	ASM_M1,	"\5",					XX,		P_86 )
IDAT(	FNSTCW,		DFLT|DNOMF,			1,		2,		R111,					"fnstcw",	ACLS_FNCNTL,ASM_M0,	"\1",					XX,		P_86 )
IDAT(	FNSTSW,		DFLT|DNOMF,			1,		2,		R111,					"fnstsw",	ACLS_FNCNTL,ASM_M1,	"\5",					XX,		P_86 )
IDAT(	FNSAVE,		DFLT|00000000,		1,		2,		R110,					"fnsave",	ACLS_FNCNTL,ASM_M2,	"\5",					XX,		P_86 )
IDAT(	FNSTENV,	DFLT|00000000,		1,		2,		R110,					"fnstenv",	ACLS_FNCNTL,ASM_M3,	"\1",					XX,		P_86 )
IDAT(	FNINIT,		DFLT|00000000,		0,		3,		0xe3,					"fninit",	ACLS_FNIMM4,ASM_M0,	"\3",					XX,		P_86 )
IDAT(	FNCLEX,		DFLT|00000000,		0,		3,		0xe2,					"fnclex",	ACLS_FNIMM4,ASM_M1,	"\3",					XX,		P_86 )
IDAT(	FNDISI,		DFLT|00000000,		0,		3,		0xe1,					"fndisi",	ACLS_FNIMM4,ASM_M2,	"\3",					XX,		P_86 )
IDAT(	FNENI,		DFLT|00000000,		0,		3,		0xe0,					"fneni",	ACLS_FNIMM4,ASM_M3,	"\3",					XX,		P_86 )
QPDAT(	FNSTSWX,	DFLT|DNOMF,			0,		3,		0xe0,					"_fnstswx ",ACLS_FNIMMW,ASM_M0,	"\7",					XX,		P_86 )	/* fnstsw ax */
TPDAT(	FNSTSWX,	DFLT|DNOMF,			0,		3,		0xe0,					"_fnstswx ",ACLS_FNIMMW,ASM_M0,	"\7",					XX,		P_86 )	/* fnstsw ax */
/*
*	suffixes on floating pseudo ops:
*		t:	exTended
*		b:	backwards (store result in st(i)
*		n:	no operands
*/
IDAT(	F2XM1,		DFLT|00000000,		0,		2,		0xf0,					"f2xm1",	ACLS_FIMM3,	ASM_M2,	"\1",					XX,		P_86 )
CDAT(	FABS,		DFLT|00000000,		0,		3,		0xe1,					"fabs",		ACLS_FIMM2,	ASM_M6,	"\1",					XX,		P_86 )
QDAT(	FCHS,		DFLT|00000000,		0,		3,		0xe0,					"fchs",		ACLS_FIMM2,	ASM_M7,	"\1",					XX,		P_86 )
IDAT(	FCLEX,		DFLT|00000000,		0,		3,		0xe2,					"fclex",	ACLS_FIMM4,	ASM_M1,	"\3",					XX,		P_86 )
IDAT(	FDECSTP,	DFLT|00000000,		0,		3,		0xf6,					"fdecstp",	ACLS_FIMM3,	ASM_M6,	"\1",					XX,		P_86 )
IDAT(	FDISI,		DFLT|00000000,		0,		3,		0xe1,					"fdisi",	ACLS_FIMM4,	ASM_M2,	"\3",					XX,		P_86 )
IDAT(	FENI,		DFLT|00000000,		0,		3,		0xe0,					"feni",		ACLS_FIMM4,	ASM_M3,	"\3",					XX,		P_86 )
IDAT(	FINCSTP,	DFLT|00000000,		0,		3,		0xf7,					"fincstp",	ACLS_FIMM3,	ASM_M5,	"\1",					XX,		P_86 )
IDAT(	FINIT,		DFLT|00000000,		0,		3,		0xe3,					"finit",	ACLS_FIMM4,	ASM_M0,	"\3",					XX,		P_86 )
CPDAT(	FLDT,		DFLT|00000000,		1,		2,		R101,					"_fldt ",		ACLS_NIL,	ASM_MNIL,	"\3",			XX,		P_86 ) /* fild long int (really ASM_M5) */
TPDAT(	FLDT,		DFLT|00000000,		1,		2,		R101,					"_fldt ",		ACLS_NIL,	ASM_MNIL,	"\3",			XX,		P_86 ) /* fild long int (really ASM_M5) */
QPDAT(	FILD8,		DFLT|00000000,		1,		2,		R101,					"_fild8 ",		ACLS_FSPEC,	ASM_M0,	"\3",				XX,		P_86 ) /* fild long int (really ASM_M5) */
TPDAT(	FILD8,		DFLT|00000000,		1,		2,		R101,					"_fild8 ",		ACLS_FSPEC,	ASM_M0,	"\3",				XX,		P_86 ) /* fild long int (really ASM_M5) */
CDAT(	FLDZ,		DFLT|00000000,		0,		3,		0xee,					"fldz",		ACLS_FIMM1,	ASM_M2,	"\1",					XX,		P_86 )
IDAT(	FLDPI,		DFLT|00000000,		0,		3,		0xeb,					"fldpi",	ACLS_FIMM1,	ASM_M4,	"\1",					XX,		P_86 )
IDAT(	FLDL2E,		DFLT|00000000,		0,		3,		0xea,					"fldl2e",	ACLS_FIMM1,	ASM_M6,	"\1",					XX,		P_86 )
IDAT(	FLDL2T,		DFLT|00000000,		0,		3,		0xe9,					"fldl2t",	ACLS_FIMM1,	ASM_M5,	"\1",					XX,		P_86 )
IDAT(	FLDLG2,		DFLT|00000000,		0,		3,		0xec,					"fldlg2",	ACLS_FIMM1,	ASM_M7,	"\1",					XX,		P_86 )
IDAT(	FLDLN2,		DFLT|00000000,		0,		3,		0xed,					"fldln2",	ACLS_FIMM2,	ASM_M0,	"\1",					XX,		P_86 )
IDAT(	FNOP,		DFLT|00000000,		0,		3,		0xd0,					"fnop",		ACLS_FIMM3,	ASM_M7,	"\1",					XX,		P_86 )
IDAT(	FPATAN,		DFLT|00000000,		0,		3,		0xf3,					"fpatan",	ACLS_FIMM3,	ASM_M1,	"\1",					XX,		P_86 )
IDAT(	FPREM,		DFLT|00000000,		0,		3,		0xf8,					"fprem",	ACLS_FIMM2,	ASM_M3,	"\1",					XX,		P_86 )
IDAT(	FPTAN,		DFLT|00000000,		0,		3,		0xf2,					"fptan",	ACLS_FIMM3,	ASM_M0,	"\1",					XX,		P_86 )
IDAT(	FRNDINT,	DFLT|00000000,		0,		3,		0xfc,					"frndint",	ACLS_FIMM2,	ASM_M4,	"\1",					XX,		P_86 )
IDAT(	FSCALE,		DFLT|00000000,		0,		0,		0xfd,					"fscale",	ACLS_FIMM2,	ASM_M2,	"\1",					XX,		P_86 )
IDAT(	FSETPM,		DFLT|00000000,		0,		3,		0xe4,					"fsetpm",	ACLS_FIMM4,	ASM_M4,	"\3",					XX,		P_286 )
IDAT(	FSQRT,		DFLT|00000000,		0,		0,		0xfa,					"fsqrt",	ACLS_FIMM2,	ASM_M1,	"\1",					XX,		P_86 )
QPDAT(	FSTSWX,		DFLT|DNOMF,			0,		3,		0xe0,					"_fstswx ",	ACLS_FIMMW,	ASM_M0,	"\7",					XX,		P_86 )	/* fstsw ax */
TPDAT(	FSTSWX,		DFLT|DNOMF,			0,		3,		0xe0,					"_fstswx ",	ACLS_FIMMW,	ASM_M0,	"\7",					XX,		P_86 )	/* fstsw ax */
CDAT(	FTST,		DFLT|00000000,		0,		3,		0xe4,					"ftst",		ACLS_FIMM1,	ASM_M0,	"\1",					XX,		P_86 )
CDAT(	FWAIT,		DFLT|00000000,		0,		2,		RNON,					"fwait",	ACLS_IMM4,	ASM_M6,	"\3",					XX,		P_86 )
IDAT(	FXAM,		DFLT|00000000,		0,		3,		0xe5,					"fxam",		ACLS_FIMM1,	ASM_M1,	"\1",					XX,		P_86 )
IDAT(	FXTRACT,	DFLT|00000000,		0,		3,		0xf4,					"fxtract",	ACLS_FIMM2,	ASM_M5,	"\1",					XX,		P_86 )
IDAT(	FYL2X,		DFLT|00000000,		0,		3,		0xf1,					"fyl2x",	ACLS_FIMM3,	ASM_M3,	"\1",					XX,		P_86 )
IDAT(	FYL2XP1,	DFLT|00000000,		0,		3,		0xf9,					"fyl2xp1",	ACLS_FIMM3,	ASM_M4,	"\1",					XX,		P_86 )
CPDAT(	FSTTP,		DFLT|00000000,		1,		2,		R111,					"_fsttp ",	ACLS_FIMMW,	ASM_M3,	"\3",					XX,		P_86 ) /* fstp st(0) */
QPDAT(	FZAP,		DFLT|00000000,		1,		2,		R111,					"_fzap ",	ACLS_FIMMW,	ASM_M3,	"\3",					XX,		P_86 ) /* fstp st(0) */
TPDAT(	FSTTP,		DFLT|00000000,		1,		2,		R111,					"_fzap ",	ACLS_FIMMW,	ASM_M3,	"\3",					XX,		P_86 ) /* fstp st(0) */
QDAT(	FLD1,		DFLT|00000000,		0,		3,		0xe8,					"fld1",		ACLS_FIMM1,	ASM_M3,	"\1",					XX,		P_86 )
CDAT(	AAA,		DUSECC|DSETCC,		0,		1,		RNON,					"aaa",		ACLS_IMM3,	ASM_M1,	"\67",					XX,		P_86 )
CDAT(	AAD,		000000|DSETCC,		0,		2,		012,					"aad",		ACLS_X0,	ASM_M2,	"\325",					XX,		P_86 )
CDAT(	AAM,		000000|DSETCC,		0,		2,		012,					"aam",		ACLS_X0,	ASM_M1,	"\324",					XX,		P_86 )
CDAT(	AAS,		DUSECC|DSETCC,		0,		1,		RNON,					"aas",		ACLS_IMM3,	ASM_M2,	"\77",					XX,		P_86 )
QDAT(	CBW,		000000|000000,		0,		1,		RNON,					"cbw",		ACLS_IMM1,	ASM_M0,	"\230",					XX,		P_86 )
CDAT(	CLC,		000000|DSETCC,		0,		1,		RNON,					"clc",		ACLS_IMM4,	ASM_M0,	"\370",					XX,		P_86 )
QDAT(	CLD,		000000|DSETCC,		0,		1,		RNON,					"cld",		ACLS_IMM2,	ASM_M1,	"\374",					XX,		P_86 )
CDAT(	CLI,		000000|DSETCC,		0,		1,		RNON,					"cli",		ACLS_IMM4,	ASM_M2,	"\372",					XX,		P_86 )
IDAT(	CLTS,		DZEROF|000000,		0,		2,		0,						"clts",		ACLS_X0,	ASM_M0,	"\6",					XX,		P_PROT|P_286 )
CDAT(	CMC,		DUSECC|DSETCC,		0,		1,		RNON,					"cmc",		ACLS_IMM4,	ASM_M1,	"\365",					XX,		P_86 )
QDAT(	CWD,		000000|000000,		0,		1,		RNON,					"cwd",		ACLS_IMM1,	ASM_M1,	"\231",					XX,		P_86 )
CDAT(	DAA,		DUSECC|DSETCC,		0,		1,		RNON,					"daa",		ACLS_IMM3,	ASM_M3,	"\47",					XX,		P_86 )
CDAT(	DAS,		DUSECC|DSETCC,		0,		1,		RNON,					"das",		ACLS_IMM3,	ASM_M4,	"\57",					XX,		P_86 )
CDAT(	HLT,		0,					0,		1,		RNON,					"hlt",		ACLS_IMM3,	ASM_M5,	"\364",					XX,		P_86 )
QPDAT(	IMUL3,		DSETCC,				2,		1,		RNON,					"_imul3 ",	ACLS_IMUL3,	ASM_M0,	"\151",					XX,		P_186 )
CPDAT(	IMUL3,		DSETCC,				2,		1,		RNON,					"_imul3 ",	ACLS_IMUL3,	ASM_M0,	"\151",					XX,		P_186 )
TPDAT(	IMUL3,		DSETCC,				2,		1,		RNON,					"_imul3 ",	ACLS_IMUL3,	ASM_M0,	"\151",					XX,		P_186 )
CDAT(	INTO,		000000|000000,		0,		1,		RNON,					"into",		ACLS_IMM3,	ASM_M6,	"\316",					XX,		P_86 )
QDAT(	IRET,		000000|DSETCC,		0,		1,		RNON,					"iret",		ACLS_IMM2,	ASM_M0,	"\317",					XX,		P_86 )
CDAT(	LAHF,		DUSECC|000000,		0,		1,		RNON,					"lahf",		ACLS_IMM2,	ASM_M6,	"\237",					XX,		P_86 )
QDAT(	LEAVE,		000000|000000,		0,		1,		RNON,					"leave",	ACLS_IMM1,	ASM_M5,	"\311",					XX,		P_186 )
CPDAT(	LJCC,		DUSECC|000000,		1,		2,		RNON,					"_ljcc ",	ACLS_NIL,	ASM_MNIL,	"\160",				XX,		P_86 )
TPDAT(	LJCC,		DUSECC|000000,		1,		2,		RNON,					"_ljcc ",	ACLS_NIL,	ASM_MNIL,	"\160",				XX,		P_86 )
			/*	long	compare	*/
QDAT(	NOP,		000000|000000,		0,		1,		RNON,					"nop",		ACLS_IMM1,	ASM_M6,	"\220",					XX,		P_86 )
QDAT(	POPA,		000000|000000,		0,		1,		R000,					"popa",		ACLS_IMM2,	ASM_M2,	"\141",					XX,		P_186 )
CDAT(	POPF,		000000|DSETCC,		0,		1,		RNON,					"popf",		ACLS_IMM2,	ASM_M5,	"\235",					XX,		P_86 )
QDAT(	PUSHA,		000000|000000,		0,		1,		R000,					"pusha",	ACLS_IMM2,	ASM_M3,	"\140",					XX,		P_186 )
QDAT(	PUSHF,		DUSECC|000000,		0,		1,		RNON,					"pushf",	ACLS_IMM2,	ASM_M4,	"\234",					XX,		P_86 )
CDAT(	SAHF,		000000|DSETCC,		0,		1,		RNON,					"sahf",		ACLS_IMM2,	ASM_M7,	"\236",					XX,		P_86 )
CPDAT(	SEG,		000000|000000,		1,		1,		RNON,					"_seg ",	ACLS_NIL,	ASM_MNIL, "\46",				XX,		P_86 )
TPDAT(	SEG,		000000|000000,		1,		1,		RNON,					"_seg ",	ACLS_NIL,	ASM_MNIL, "\46",				XX,		P_86 )
CDAT(	STC,		000000|DSETCC,		0,		1,		RNON,					"stc",		ACLS_IMM4,	ASM_M3,	"\371",					XX,		P_86 )
CDAT(	STD,		000000|DSETCC,		0,		1,		RNON,					"std",		ACLS_IMM4,	ASM_M4,	"\375",					XX,		P_86 )
CDAT(	STI,		000000|DSETCC,		0,		1,		RNON,					"sti",		ACLS_IMM4,	ASM_M5,	"\373",					XX,		P_86 )
CDAT(	WAIT,		000000|000000,		0,		1,		RNON,					"wait",		ACLS_IMM4,	ASM_M6,	"\233",					XX,		P_86 )
QPDAT(	NJCC,		XX,					XX,		XX,		XX,						"_njcc ",	ACLS_JCC,	ASM_M1,	XX,						XX,		XX )
TPDAT(	NJCC,		XX,					XX,		XX,		XX,						"_njcc ",	ACLS_JCC,	ASM_M1,	XX,						XX,		XX )
IDAT(	_EMIT,		000000|00000000,	1,		1,		RNON,					"_emit",	ACLS_INT,	ASM_M1,	"",						XX,		P_86 )
/* prefixes { */
/*
**	inline assembler needs to return a different token for prefixes.  These
**	should be kept together so inline assembler can quickly check if we
**	have a prefix or not.  These could be mapped instead of being position
**	dependent, but we are being space conscious.  M00SPACE
*/
#define	ASM_OPREFIX0	ASM_OREP
QDAT(	REP,		000000|000000,		0,		1,		RNON,					"rep",		ACLS_IMM1,	ASM_M2,	"\363",					XX,		P_86 )
CDAT(	LOCK,		000000|000000,		0,		1,		RNON,					"lock",		ACLS_IMM4,	ASM_M7,	"\360",					XX,		P_86 )
QDAT(	REPNZ,		000000|000000,		0,		1,		RNON,					"repnz",	ACLS_IMM1,	ASM_M4,	"\362",					XX,		P_86 )
QDAT(	REPZ,		000000|000000,		0,		1,		RNON,					"repz",		ACLS_IMM1,	ASM_M3,	"\363",					XX,		P_86 )
#define	ASM_OPREFIXN	ASM_OREPZ
/* prefixes } */

/* pseudo-ops (not seen by assembler) */
QPDAT(	POP1,		XX,					XX,		XX,		XX,						"_POP1 ",	ACLS_PSE1,	ASM_M1,	NULL,					XX,		P_86 )
TPDAT(	POP1,		XX,					XX,		XX,		XX,						"_POP1 ",	ACLS_PSE1,	ASM_M1,	NULL,					XX,		P_86 )
QPDAT(	POP2,		XX,					XX,		XX,		XX,						"_POP2 ",	ACLS_PSE1,	ASM_M2,	NULL,					XX,		P_86 )
TPDAT(	POP2,		XX,					XX,		XX,		XX,						"_POP2 ",	ACLS_PSE1,	ASM_M2,	NULL,					XX,		P_86 )
QPDAT(	POPFLT1,	XX,					XX,		XX,		XX,						"_POPFLT1 ",	ACLS_PSE1,	ASM_M7,	NULL,				XX,		P_86 )
TPDAT(	POPFLT1,	XX,					XX,		XX,		XX,						"_POPFLT1 ",	ACLS_PSE1,	ASM_M7,	NULL,				XX,		P_86 )
QPDAT(	POPFLT2,	XX,					XX,		XX,		XX,						"_POPFLT2 ",	ACLS_PSE2,	ASM_M0,	NULL,				XX,		P_86 )
TPDAT(	POPFLT2,	XX,					XX,		XX,		XX,						"_POPFLT2 ",	ACLS_PSE2,	ASM_M0,	NULL,				XX,		P_86 )
QPDAT(	POPINC,		XX,					XX,		XX,		XX,						"_POPINC ",		ACLS_PSE1,	ASM_M6,	XX,					XX,		P_86 )
TPDAT(	POPINC,		XX,					XX,		XX,		XX,						"_POPINC ",		ACLS_PSE1,	ASM_M6,	XX,					XX,		P_86 )
QPDAT(	POPINC1,	XX,					XX,		XX,		XX,						"_POPINC1 ",	ACLS_PSE1,	ASM_M4,	NULL,				XX,		P_86 )
TPDAT(	POPINC1,	XX,					XX,		XX,		XX,						"_POPINC1 ",	ACLS_PSE1,	ASM_M4,	NULL,				XX,		P_86 )
QPDAT(	POPUNS,		XX,					XX,		XX,		XX,						"_POPUNS ",		ACLS_PSE1,	ASM_M5,	XX,					XX,		P_86 )
TPDAT(	POPUNS,		XX,					XX,		XX,		XX,						"_POPUNS ",		ACLS_PSE1,	ASM_M5,	XX,					XX,		P_86 )
QPDAT(	POPUNS1,	XX,					XX,		XX,		XX,						"_POPUNS1 ",	ACLS_PSE1,	ASM_M3,	NULL,				XX,		P_86 )
TPDAT(	POPUNS1,	XX,					XX,		XX,		XX,						"_POPUNS1 ",	ACLS_PSE1,	ASM_M3,	NULL,				XX,		P_86 )

/* pseudo-ops (seen by assembler ) */
QPDAT(	PENTRY,		XX,					1,		XX,		XX,						"_PENTRY ",	ACLS_LABEL,	ASM_M1,	NULL,					XX,		P_86 )
TPDAT(	PENTRY,		XX,					1,		XX,		XX,						"_PENTRY ",	ACLS_LABEL,	ASM_M1,	NULL,					XX,		P_86 )
QPDAT(	PLAB,		XX,					1,		XX,		XX,						"_PLAB ",	ACLS_LABEL,	ASM_M0,	NULL,					XX,		P_86 )
TPDAT(	PLAB,		XX,					1,		XX,		XX,						"_PLAB ",	ACLS_LABEL,	ASM_M0,	NULL,					XX,		P_86 )
QPDAT(	PWINFRM,	XX,					XX,		XX,		XX,						"_PWINFRM ",	ACLS_PSE2,	ASM_M2,	NULL,				XX,		P_86 )
TPDAT(	PWINFRM,	XX,					XX,		XX,		XX,						"_PWINFRM ",	ACLS_PSE2,	ASM_M2,	NULL,				XX,		P_86 )
QPDAT(	PLINNUM,	XX,					XX,		XX,		XX,						"_PLINNUM ",	ACLS_PSE2,	ASM_M1,	NULL,				XX,		P_86 )
TPDAT(	PLINNUM,	XX,					XX,		XX,		XX,						"_PLINNUM ",	ACLS_PSE2,	ASM_M1,	NULL,				XX,		P_86 )
QPDAT(	PFILNAM,	XX,					XX,		XX,		XX,						"_PFILNAM ",	ACLS_PSE2,	ASM_M3,	NULL,				XX,		P_86 )
TPDAT(	PFILNAM,	XX,					XX,		XX,		XX,						"_PFILNAM ",	ACLS_PSE2,	ASM_M3,	NULL,				XX,		P_86 )
QPDAT(	PBLKSTR,	XX,					XX,		XX,		XX,						"_PBLKSTR ",	ACLS_PSE2,	ASM_M4,	NULL,				XX,		P_86 )
TPDAT(	PBLKSTR,	XX,					XX,		XX,		XX,						"_PBLKSTR ",	ACLS_PSE2,	ASM_M4,	NULL,				XX,		P_86 )
QPDAT(	PBLKEND,	XX,					XX,		XX,		XX,						"_PBLKEND ",	ACLS_PSE2,	ASM_M5,	NULL,				XX,		P_86 )
TPDAT(	PBLKEND,	XX,					XX,		XX,		XX,						"_PBLKEND ",	ACLS_PSE2,	ASM_M5,	NULL,				XX,		P_86 )
QPDAT(	PDATA,		000000|000000,		2,		0,		0,						"_PDATA ",	ACLS_PSE1,	ASM_M0,	XX,						XX,		P_86 )
TPDAT(	PDATA,		000000|000000,		2,		0,		0,						"_PDATA ",	ACLS_PSE1,	ASM_M0,	XX,						XX,		P_86 )

CPDAT(	EPOP,		000000|000000,		1,		1,		R000,					"_epop ",	ACLS_NIL,	ASM_MNIL,"\217\130\7",			XX,		P_86 )
TPDAT(	EPOP,		000000|000000,		1,		1,		R000,					"_epop ",	ACLS_NIL,	ASM_MNIL,"\217\130\7",			XX,		P_86 )
CPDAT(	EPUSH,		000000|000000,		1,		1,		R110,					"_epush ",	ACLS_NIL,	ASM_MNIL,"\377\120\6\150",		XX,		P_86 )
TPDAT(	EPUSH,		000000|000000,		1,		1,		R110,					"_epush ",	ACLS_NIL,	ASM_MNIL,"\377\120\6\150",		XX,		P_86 )
CPDAT(	DATA,		000000|000000,		2,		0,		0,						"_data ",	ACLS_PSE1,	ASM_M0,	"",						XX,		P_86 )
TPDAT(	DATA,		000000|000000,		2,		0,		0,						"_data ",	ACLS_PSE1,	ASM_M0,	"",						XX,		P_86 )
CPDAT(	RPUSH,		000000|000000,		1,		1,		R110,					"_rpush ",	ACLS_NIL,	ASM_MNIL,"\377\120\6\150",		XX,		P_86 )
TPDAT(	RPUSH,		000000|000000,		1,		1,		R110,					"_rpush ",	ACLS_NIL,	ASM_MNIL,"\377\120\6\150",		XX,		P_86 )
CPDAT(	SCC,		000000|DUSECC|DZEROF,1, 	2,		RNON,					"_scc ",	ACLS_SCC,	ASM_M0, "\220",					XX, 	P_386 )
QPDAT(	SCC,		000000|DUSECC|DZEROF,1, 	2,		RNON,					"_scc ",	ACLS_SCC,	ASM_M0, "\220",					XX, 	P_386 )
TPDAT(	SCC,		000000|DUSECC|DZEROF,1, 	2,		RNON,					"_scc ",	ACLS_SCC,	ASM_M0, "\220",					XX, 	P_386 )
QDAT(	LSS,		000000|DZEROF,		2,		2,		RNON,					"lss",		ACLS_BINOP2,ASM_M0, "\262", 				XX, 	P_386 )
QDAT(	LFS,		000000|DZEROF,		2,		2,		RNON,					"lfs",		ACLS_BINOP2,ASM_M1, "\264",					XX, 	P_386 )
QDAT(	LGS,		000000|DZEROF,		2,		2,		RNON,					"lgs",		ACLS_BINOP2,ASM_M2, "\265",					XX, 	P_386 )
CPDAT(	IMUL2,		000000|DSETCC|DZEROF,2, 	2,		RNON,					"_imul2 ",	ACLS_X2B,	ASM_M4, "\257", 				XX, 	P_386 )
QPDAT(	IMUL2,		000000|DSETCC|DZEROF,2, 	2,		RNON,					"_imul2 ",	ACLS_X2B,	ASM_M4, "\257", 				XX, 	P_386 )
TPDAT(	IMUL2,		000000|DSETCC|DZEROF,2, 	2,		RNON,					"_imul2 ",	ACLS_X2B,	ASM_M4, "\257", 				XX, 	P_386 )
QDAT(	MOVZX,		000000|DZEROF,		2,		2,		0,						"movzx",	ACLS_MOVX,	ASM_M0, "\266",					XX, 	P_386 )
CPDAT(	MOVZXW,		000000|DZEROF,		2,		2,		0,						"_movzxw ",	ACLS_MOVX,	ASM_M0, "\267",					XX, 	P_386 )
TPDAT(	MOVZXW,		000000|DZEROF,		2,		2,		0,						"_movzxw ",	ACLS_MOVX,	ASM_M0, "\267",					XX, 	P_386 )
QDAT(	MOVSX,		000000|DZEROF,		2,		2,		0,						"movsx",	ACLS_MOVX,	ASM_M1, "\276",					XX, 	P_386 )
CPDAT(	MOVSXW,		000000|DZEROF,		2,		2,		0,						"_movsxw ",	ACLS_MOVX,	ASM_M1, "\277",					XX, 	P_386 )
TPDAT(	MOVSXW,		000000|DZEROF,		2,		2,		0,						"_movsxw ",	ACLS_MOVX,	ASM_M1, "\277",					XX, 	P_386 )
CPDAT(	JMP3216,	000000|000000,		1,		1,		RNON,					"_jmp3216 ",ACLS_JMP,	ASM_M1, "",						XX, 	P_386 )
TPDAT(	JMP3216,	000000|000000,		1,		1,		RNON,					"_jmp3216 ",ACLS_JMP,	ASM_M1, "",						XX, 	P_386 )
CPDAT(	JMP1632,	000000|000000,		1,		1,		RNON,					"_jmp1632 ",ACLS_JMP,	ASM_M2, "",						XX, 	P_386 )
TPDAT(	JMP1632,	000000|000000,		1,		1,		RNON,					"_jmp1632 ",ACLS_JMP,	ASM_M2, "",						XX, 	P_386 )
IDAT(	BSF,		000000|DZEROF,		2,		2,		XX, 					"bsf",		ACLS_X2B,	ASM_M2, "\274",					XX, 	P_386 )
IDAT(	BSR,		000000|DZEROF,		2,		2,		XX, 					"bsr",		ACLS_X2B,	ASM_M3, "\275",					XX, 	P_386 )
IDAT(	SHLD,		000000|DZEROF,		2,		2,		XX, 					"shld", 	ACLS_SHLD,	ASM_M0, "\244",					XX, 	P_386 )
IDAT(	SHRD,		000000|DZEROF,		2,		2,		XX, 					"shrd", 	ACLS_SHLD,	ASM_M2, "\254",					XX, 	P_386 )
IDAT(	BT, 		000000|DZEROF, 		2,		2,		R100,					"bt",		ACLS_BTEST, ASM_M0, "\272",					XX, 	P_386 )
IDAT(	BTS,		000000|DZEROF,		2,		2,		R101,					"bts",		ACLS_BTEST, ASM_M1, "\272",					XX, 	P_386 )
IDAT(	BTR,		000000|DZEROF, 		2,		2,		R110,					"btr",		ACLS_BTEST, ASM_M2, "\272",					XX, 	P_386 )
IDAT(	BTC,		000000|DZEROF,		2,		2,		R111,					"btc",		ACLS_BTEST, ASM_M3, "\272",					XX, 	P_386 )
CPDAT(	MOVSR,		XX, 				2,		XX, 	XX, 					"_movsr ",	ACLS_MOVSR, ASM_M0, "\40",					XX, 	P_386 )
QPDAT(	MOVSR,		XX, 				2,		XX, 	XX, 					"_movsr ",	ACLS_MOVSR, ASM_M0, "\40",					XX, 	P_386 )
TPDAT(	MOVSR,		XX, 				2,		XX, 	XX, 					"_movsr ",	ACLS_MOVSR, ASM_M0, "\40",					XX, 	P_386 )
QDAT(	FUCOM,		DFLT|00000000,		2,		XX,		R100,					"fucom",	ACLS_FOP,	ASM_M2,	"\0",					XX,		P_386 )
QDAT(	FUCOMP,		DFLT|00000000,		2,		XX,		R101,					"fucomp",	ACLS_FOP,	ASM_M3,	"\0",					XX,		P_386 )
CDAT(	FUCOMPP,	DFLT|00000000,		2,		XX,		0xe9,					"fucompp",	ACLS_FIMMW,	ASM_M4,	"\6",					XX,		P_386 )
IDAT(	FPREM1,		DFLT|00000000,		0,		3,		0xf5,					"fprem1",	ACLS_FIMM5,	ASM_M3,	"\1\365",				XX,		P_386 )
IDAT(	FSINCOS,	DFLT|00000000,		0,		3,		0xfb,					"fsincos",	ACLS_FIMM5,	ASM_M2,	"\1\373",				XX,		P_386 )
IDAT(	FSIN,		DFLT|00000000,		0,		3,		0xfe,					"fsin", 	ACLS_FIMM5,	ASM_M0,	"\1\376",				XX,		P_386 )
IDAT(	FCOS,		DFLT|00000000,		0,		3,		0xff,					"fcos", 	ACLS_FIMM5,	ASM_M1,	"\1\377",				XX,		P_386 )
IDAT(	BSWAP,		000000|DZEROF,		1,		XX, 	XX, 					"bswap",	ACLS_BSWAP,	ASM_M0,	"\310",					XX, 	P_486 )
IDAT(	XADD,		DZEROF|DSETCC,		2,		XX, 	XX, 					"xadd",		ACLS_XCHG2,	ASM_M0,	"\300",					XX, 	P_486 )
IDAT(	CMPXCHG,	DZEROF|DSETCC,		2,		XX, 	XX, 					"cmpxchg",	ACLS_XCHG2,	ASM_M1,	"\240",					XX, 	P_486 )
IDAT(	INVD,		DZEROF|000000,		0,		XX, 	XX, 					"invd",		ACLS_X0,	ASM_M3,	"\010",					XX, 	P_486 )
IDAT(	WBINVD,		DZEROF|000000,		0,		XX, 	XX, 					"wbinvd",	ACLS_X0,	ASM_M4,	"\011",					XX, 	P_486 )
IDAT(	INVLPG,		DZEROF|000000,		1,		XX, 	XX, 					"invlpg",	ACLS_INVLPG,ASM_M0,	"\001",					XX, 	P_486 )
QDAT(	XCWDE,		000000|000000,		0,		1,		RNON,					"cwde ",	ACLS_IMM1,	ASM_M1,	"\231",					XX,		P_86 )
QDAT(	XCDQ,		000000|000000,		0,		1,		RNON,					"cdq ",		ACLS_IMM1,	ASM_M0,	"\230",					XX,		P_86 )
CPDAT(	SCMPS,		000000|DSETCC,		0,		1,		RNON,					"cmps ",	ACLS_STR,	ASM_M1,	"\246",					XX,		P_86 )
CPDAT(	SLODS,		000000|000000,		0,		1,		RNON,					"lods ",	ACLS_STR,	ASM_M3,	"\254",					XX,		P_86 )
CPDAT(	SMOVS,		000000|000000,		0,		1,		RNON,					"movs ",	ACLS_STR,	ASM_M0,	"\244",					XX,		P_86 )
CPDAT(	SOUTS,		000000|000000,		0,		1,		RNON,					"outs ",	ACLS_STR,	ASM_M6,	"\156",					XX,		P_186 )
CPDAT(	SXLAT,		000000|000000,		0,		1,		RNON,					"xlat ",	ACLS_IMM3,	ASM_M0,	"\327",					XX,		P_86 )
IDAT(	EMMS,		000000|000000,		0,		XX,		XX,						"emms",		ACLS_X0,	ASM_M5,	"\167",					XX,		P_MM86 )
IDAT(	MOVMM,		000000|000000,		2,		XX,		XX,						"movmm ",	ACLS_MMMOV,	ASM_M0,	"",						XX,		P_MM86 )
IDAT(	PACKSS,		000000|000000,		2,		XX,		XX,						"packss ",	ACLS_MMPACK,ASM_M0,	"",						XX,		P_MM86 )
IDAT(	PACKUS,		000000|000000,		2,		XX,		XX,						"packus ",	ACLS_MMPACK,ASM_M1,	"",						XX,		P_MM86 )
IDAT(	PADD,		000000|000000,		2,		XX,		XX,						"padd ",	ACLS_MMMATH1,ASM_M0,"",						XX,		P_MM86 )
IDAT(	PADDS,		000000|000000,		2,		XX,		XX,						"padds ",	ACLS_MMMATH1,ASM_M1,"",						XX,		P_MM86 )
IDAT(	PADDUS,		000000|000000,		2,		XX,		XX,						"paddus ",	ACLS_MMMATH1,ASM_M2,"",						XX,		P_MM86 )
IDAT(	PAND,		000000|000000,		2,		XX,		XX,						"pand",		ACLS_MMMATH2,ASM_M0,"",						XX,		P_MM86 )
IDAT(	PANDN,		000000|000000,		2,		XX,		XX,						"pandn",	ACLS_MMMATH2,ASM_M1,"",						XX,		P_MM86 )
IDAT(	PCMPEQ,		000000|000000,		2,		XX,		XX,						"pcmpeq ",	ACLS_MMPACK,ASM_M2,	"",						XX,		P_MM86 )
IDAT(	PCMPGT,		000000|000000,		2,		XX,		XX,						"pcmpgt ",	ACLS_MMPACK,ASM_M3,	"",						XX,		P_MM86 )
IDAT(	PMADD,		000000|000000,		2,		XX,		XX,						"pmadd ",	ACLS_MMMATH2,ASM_M2,"",						XX,		P_MM86 )
IDAT(	PMULH,		000000|000000,		2,		XX,		XX,						"pmulh ",	ACLS_MMMATH2,ASM_M3,"",						XX,		P_MM86 )
IDAT(	PMULL,		000000|000000,		2,		XX,		XX,						"pmull ",	ACLS_MMMATH2,ASM_M4,"",						XX,		P_MM86 )
IDAT(	POR,		000000|000000,		2,		XX,		XX,						"por",		ACLS_MMMATH2,ASM_M5,"",						XX,		P_MM86 )
IDAT(	PSLL,		000000|000000,		2,		XX,		XX,						"psll",		ACLS_MMSHIFT,ASM_M0,"",						XX,		P_MM86 )
IDAT(	PSRA,		000000|000000,		2,		XX,		XX,						"psra",		ACLS_MMSHIFT,ASM_M1,"",						XX,		P_MM86 )
IDAT(	PSRL,		000000|000000,		2,		XX,		XX,						"psrl",		ACLS_MMSHIFT,ASM_M2,"",						XX,		P_MM86 )
IDAT(	PSUB,		000000|000000,		2,		XX,		XX,						"psub ",	ACLS_MMMATH1,ASM_M3,"",						XX,		P_MM86 )
IDAT(	PSUBS,		000000|000000,		2,		XX,		XX,						"psubs ",	ACLS_MMMATH1,ASM_M4,"",						XX,		P_MM86 )
IDAT(	PSUBUS,		000000|000000,		2,		XX,		XX,						"psubus ",	ACLS_MMMATH1,ASM_M5,"",						XX,		P_MM86 )
IDAT(	PUNPCKH,	000000|000000,		2,		XX,		XX,						"punpckh ",	ACLS_MMPACK,ASM_M4,	"",						XX,		P_MM86 )
IDAT(	PUNPCKL,	000000|000000,		2,		XX,		XX,						"punpckl ",	ACLS_MMPACK,ASM_M5,	"",						XX,		P_MM86 )
IDAT(	PXOR,		000000|000000,		2,		XX,		XX,						"pxor",		ACLS_MMMATH2,ASM_M6,"",						XX,		P_MM86 )
CPDAT(	LASTOP,		000000|000000,		0,		0,		0,						"_lastop ",	ACLS_NIL,	ASM_MNIL,	"",					XX,		P_86 )
TPDAT(	LASTOP,		000000|000000,		0,		0,		0,						"_lastop ",	ACLS_NIL,	ASM_MNIL,	"",					XX,		P_86 )
/* opcodes GROUP } */
/*   	op,			dope,			num_oprs,	osize,	byte2					name,	ACLS,		ASM,		template,				XX,		machine */

/*
**	make sure this is the last opcode depending upon which DAT macro is active
*/
// M00MERGE: ASM_OOPN bug in two-pass?
#define ASM_OOPN		ASM_OLASTOP

#undef	QDAT
#undef	CDAT
#undef	IDAT
#undef	TPDAT
#undef	QPDAT
#undef	CPDAT
