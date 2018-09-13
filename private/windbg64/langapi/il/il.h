/* SCCSWHAT( "@(#)il.h	3.9 89/12/06 13:45:39	" )*/
/*
 * Special defined constants and information structures needed by both the p1-p2
 * IL and the p2-p3 IL
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

typedef unsigned long symkey_t;

#define MAXSYMKEY	0x7fffffff		// to accomodate compressed il output
#if 1 || _VC_VER >= 300
#define MAXLOCALSYMKEY	0x3fffff
#define MINLOCALSYMKEY	(MAXLOCALSYMKEY - 0xffff)
#define LOCALILKEYBIAS 	(MAXLOCALSYMKEY - 0x7f)
#define MINGLOBALSYMKEY 1
#define MAXGLOBALSYMKEY	(MINLOCALSYMKEY - 1)
#endif
#define MAXSBRKEY		0xffffff
#define MINEXTSBRKEY	0x8000

#if FARLIST
#	define FAR_DATA far
#else
#	define FAR_DATA
#endif

#if !defined(CONST)
#	define CONST
#endif

#if 0
#if !defined(NODEBUG) && !defined(MDEBUG)
#define VERS_DEBUG 1
#endif
#endif

#define END_OF_EXPRESSION 'X'

#if VERS_DEBUG
# define	MAC_STRING(X)	#X,
#else
# define	MAC_STRING(X)
#endif
/* expression IL primitive data types */
#define P_UNDEFINED 0
#define P_SINTEGER 1
#define P_UINTEGER 2
#define P_ADDRESS 3
#define P_HADDRESS 4
#define P_REAL 5
#define P_MULTI_BYTE 6
#define P_VOID 7
#define P_COMPLEX  8
#define P_LOGICAL  9
#define P_CREAL 10
#define P_F16ADDRESS	11
#define P_MAX  15
#define PRIM_TYPE unsigned short
#define P_UNION 16
#define P_VOLATILE	0x1000
#define P_CONST		0x2000
#define P_RESTRICT	0x4000
#define P_SPARE		0x4000
#define P_ALIGNSHFT	9
#define P_ALIGNMSK	0x0E00
#define P_SIZESHFT	4
#define P_SIZEMSK	0x01F0
#define P_TYPESHFT	0
#define P_TYPEMSK	0x000F

/* variable base types */
#define BA_UNDEFINED 0
#define BA_DYNAMIC 1
#define BA_STATIC 2
#define BA_PARAMS 3
#define BA_CONSTANT 4
#define BA_SEPARATE 5
#define BA_BSS 6			 /* introduced by P2 */
#define BA_MAX 7
#define BA_FR_DATA 8			 /* FORTRAN runtime data segment */
#define BASE_TYPE unsigned short

/* Base visibility class */
#define CLASS_UNDEFINED 0
#define CLASS_DEF 		1
#define CLASS_REF 		2
#define CLASS_COM 		3
#define CLASS_LOC 		4
#define CLASS_EQU 		5
#define CLASS_EXPDEF 	6	// Old __export
#define CLASS_DLLEXPDEF 7	// New __declspec(dllexport)  M00GEN Need seperate REF?
#define CLASS_DLLIMPREF 8	// __declspec(dllimport)
#define CLASS_DLLEXPCOM	9	// __declspec(dllexport), uninitialized, in C!
#define CLASS_MAX 	   10
#define BASE_CLASS unsigned short

/* Label reference field -- bit patterns needed rather than enumerations */
typedef enum
	{
	REF_UNREFERENCED = 0,
	REF_FORWARD = 1,
	REF_BACKWARD = 2,
	REF_OUT_OF_SCOPE = 4
	} REFERENCE;

typedef enum /* procedure entry visibility */
	{
	EN_UNDEFINED = 0,
	EN_LOCAL = 1,
	EN_ST_REF = 2,
	EN_ST_DEF = 3,
	EN_GL_REF = 4,
	EN_GL_DEF = 5,
	EN_GL_EXT = 6,
	EN_GL_EXPORT = 7,		// __export.  Potentially different from __declspec(dllexport)
	EN_EXPORT = 7,
	EN_ST_EXPORT = 8,		// M00MERGE: Is this used by anybody?
	EN_DLLEXPORT = 9,		// __declspec(dllexport)
	EN_DLLIMPORT = 10,		// __declspec(dllimport)
	} VIS_ENTRY;

/*
 * Data sizes
 */
#define BYTE_SIZE	1
#define WORD_SIZE	2
#define DWORD_SIZE	4
#define QWORD_SIZE	8
#define SINGLE_SIZE	MACH_FLOAT
#define DOUBLE_SIZE	MACH_DOUBLE
#define QUAD_SIZE	(MACH_DOUBLE * 2)
#define TREAL_SIZE	MACH_TREAL
#define TCOMP_SIZE	(MACH_TREAL * 2)
#define SADDR_SIZE	MACH_NEAR
#define LADDR_SIZE	MACH_FAR

/* this is the size of a long double (temp real) as passed
 * in the ail. Because fp constants are passed in the ail
 * as length preceeded strings, and sizeof(Srealt_t) = 12. */
#define AIL_TREAL_SIZE 12


/*
 * DEBUG_IL definitions
 */
#define D_COMPLEX 0		/* expression too complex to type */
#define D_FIRSTKEY 10	/* first key available to P1 */

/*
 * bitfield definitions for the attribute/use field in the symbol
 * table entries.  Some attributes are used by SSR_NAME records (N),
 * some by SSR_ENTRY (E), and some by both (NE).
 */

/*
Some bits need to be in the low byte of SSR_NAME, because we want them
passed through to p3 (eg, AA_INIT).  Some bits need to be in the low byte of
SSR_ENTRY because they have to be on the calltype.

Should AA_RETVAL be on the low byte?  AA_SYMOUT?
Have we ever considered adding an SSR_STRING?
*/

#if _VC_VER >= 300
#define AA_NODEBUG			0x00010000	// used for ICC
#define AA_COMDAT_LARGEST	0x00020000	// used for RTTI
#define AA_COMDAT_NEWEST	0x00040000	// reserved for ICC
#define AA_BIASOFFSET		0x00080000	// for RTTI
#endif

/* SSR_NAME */
#define AA_REFERENCED	0x8000  /* p2 internal, for transitive refs */
#define AA_SYMOUT		0x8000	/* p3 internal */
#define AA_REGISTER		0x4000
#define AA_ADDRESSED	0x2000
#define AA_REGARG		0x1000	/* p2 internal */
#define AA_ALLOC		0x0800	/* p2 internal */
#define AA_PUBDECL		0x1000	/* p3 internal */
#define AA_THREAD		0x1000	// Place in thread-local storage: P1->P2
#define AA_CSTRING		0x0400
#define AA_SEG			0x0200
#define AA_USED 		0x0100
#define AA_INIT 		0x0080
#define	AA_INSTANTIATE	0x0040  // Must instantiate this (COMDAT) variable
#define AA_COMDAT		0x0020	/* also used for SSR_ENTRY symbols */
#if VERSP_MIPS
#define AA_OLDSTYLEFORMAL	0x0010	// old style formal parameter
#endif
#if VERSP_ALPHA || VERSP_MIPS
#define AA_CDTOR_PARAM		0x0008	// parameter symbol has a constructor or destructor
#endif

/* SSR_ENTRY */
#define AA_KILLOPT		0x8000
#define AA_LOADDS		0x4000	// OBSOLETE?  Not found in frontend
#define AA_SYSAPI		0x4000
#define AA_RETVAL		0x2000	// OBSOLETE?  Not found in frontend
#define AA_TRY			0x1000
#define AA_NEARCALL		0x0800	// OBSOLETE?  Not found in frontend	/* p2 -> p3 */
#define AA_NOTHROW		0x0800	// __declspec(nothrow), or excpt spec "throw()"
#define AA_NAKED		0x0400	// __declspec(naked), or thunks
#define AA_SEG			0x0200
#define AA_SETJMP 		0x0100	// OBSOLETE?  Not found in frontend
#define AA_SAVEREGS 	0x0080	// OBSOLETE?  Not found in frontend
#define AA_VARARGS		0x0040

#define AA_LSHFT		2
#define AA_LMSK 		(0x7 << AA_LSHFT)
#define AA_CALLTYPE		AA_LMSK
#define AA_LDEFAULT 	(0 << AA_LSHFT)
#define AA_C			(0 << AA_LSHFT)
#define AA_OLDCALL		(1 << AA_LSHFT) /* VERS_N10 only */
#define AA_FASTCALL 	(1 << AA_LSHFT)
#define AA_PASCAL		(2 << AA_LSHFT)
#define AA_FORTRAN		(2 << AA_LSHFT)
#define AA_JAVA   		(2 << AA_LSHFT)
#define AA_INTERRUPT	(3 << AA_LSHFT)
#define AA_CPP			(3 << AA_LSHFT) /* MIPS only, used in SSR_HEADER */
#define AA_STDCALL		(4 << AA_LSHFT)
#define AA_THISCALL 	(5 << AA_LSHFT)
#define AA_SYSCALL		(6 << AA_LSHFT)
#define AA_CNOPROTO		(7 << AA_LSHFT)

#define AA_MSHFT		0
#define AA_MMSK 		(0x3 << AA_MSHFT)
#define AA_NFH	 		(0x3 << AA_MSHFT)
#define AA_NEAR 		(1 << AA_MSHFT)
#define AA_FAR			(2 << AA_MSHFT)
#define AA_HUGE 		(3 << AA_MSHFT)
#define AA_FAR16		(3 << AA_MSHFT)
/*
 * definitions of p1-p2 IL dope vector and extraction macros
 */
typedef struct
	{
#if VERS_DEBUG
	char *d_name;		/* opcode name -- opcodes.h is generated from this */
#endif
	short d_node;		/* node attributes -- what operations are allowed */
	char *d_attr;		/* IL attributes -- how to read IL stream */
	} DOPEVECT ;

#ifndef SEPDS_DEBUG
extern CONST DOPEVECT expdope[];	/* universal declaration so macros will work */
#else
extern CONST DOPEVECT far expdope[];	/* far if debugging in separate segment */
#endif

#if CC_QC2PASS
extern CONST unsigned char	expdope2[];	/* dope vector for QC node attribute */
#endif	/* CC_QC2PASS */

/* bitfield definitions for the node attributes in the dope vector */

#define NODE_TYPE		0x3		/* node type */
#define COMM			0x4		/* is the node commutative ? */
#define SORT			0x8		/* can the node be sorted ? */
#define RELA			0x10	/* is this a relational operator ? */
#define FOLD			0x20	/* constant folding operation allowed */
#define COLEAF			0x40	/* coalesced leaf */
#define CSET			0x80	/* commutativity set : +,-,-' */
#define CSEE			0x100	/* Node can be used with CSE. */
#define KILL			0x200	/* Node kills CSE. */
#define LVALUE			0x400	/* ASSIG or OPextract */
#define ASSIG			0x800	/* is an assignment */
#define ENDEXPR 		0x1000	/* end of expression */
#define HAS_RC			0x2000	/* has rchild */
#define CSER			0x4000	/* look below for CSE */
/* M00UNUSED in P1 */
#define OPEQ			0x8000	/* can be transformed into opeq (e.g. +=, -=) */
#define ADDR_ON_LEFT	LVALUE

/* access to the fields of the dope vector */

#if VERS_DEBUG
#define D_NAME(op) (expdope[(int)(op)].d_name)
#endif
#define D_ATTRIBUTES(op) (expdope[(int)(op)].d_attr)
#define D_TYPE(op) (expdope[(int)(op)].d_node & NODE_TYPE)
#define D_COMM(op) (expdope[(int)(op)].d_node & COMM)
#define D_SORT(op) (expdope[(int)(op)].d_node & SORT)
#define D_RELA(op) (expdope[(int)(op)].d_node & RELA)
#define D_FOLD(op) (expdope[(int)(op)].d_node & FOLD)
#define D_COLEAF(op) (expdope[(int)(op)].d_node & COLEAF)
#define D_CSET(op) (expdope[(int)(op)].d_node & CSET)
#define D_CSEE(op) (expdope[(int)(op)].d_node & CSEE)
#define D_KILL(op) (expdope[(int)(op)].d_node & KILL)
#define D_LVALUE(op) (expdope[(int)(op)].d_node & LVALUE)
#define D_ASSIG(op) (expdope[(int)(op)].d_node & ASSIG)
#define D_ENDEXPR(op) (expdope[(int)(op)].d_node & ENDEXPR)
#define D_HAS_RC(op) (expdope[(int)(op)].d_node & HAS_RC)
#define D_CSER(op) (expdope[(int)(op)].d_node & CSER)

#define NODETYPE(n) ((int)D_TYPE(OPCODE(n)))
#define IS_LEAF(n) (NODETYPE(n) == LEAF)
#define HAS_RCHILD(n) (D_HAS_RC(OPCODE(n)))
#define HAS_LCHILD(n) (NODETYPE(n) != LEAF)

/*
 * definition of the information tables used to hold information on all the
 * il streams, including record type names, attributes for reading/writing
 * those records, and size information for record allocation within the
 * pass which needs those records.  The only IL stream which does not use
 * this structure to hold this information is the p1-p2 expression il stream
 * which does not need the size information, but does need other information.
 * This information is carried in a DOPEVECT table, defined above.
 */

typedef struct IL_INFO {
#if VERS_DEBUG
	char *name;	 	/* record type names */
#endif
	char FAR_DATA *attrs;	/* IL attributes - how to read/write IL stream */
	short size;		/* allocation size for record of this type */
	} IL_INFO;
#if _VC_VER >= 300
extern CONST IL_INFO symdope[];	/* universal declaration so macros will work */
extern  CONST IL_INFO	Pragma_attrs[];
extern	CONST IL_INFO	Init_info[];
#else
extern CONST IL_INFO symdope[22];	/* universal declaration so macros will work */
extern  CONST IL_INFO	Pragma_attrs[38];
extern	CONST IL_INFO	Init_info[14];
#endif

#define IL_NAME(p)	(symdope[(int)(p)].name)
#define IL_ATTRS(p) (symdope[(int)(p)].attrs)
#define IL_SIZE(p)	(symdope[(int)(p)].size)

#if _VC_VER < 300
/* maximum key numbers for each pass */
#define KY_P1MAX	20000
#define KY_P2MAX	40000
#define KY_P3MAX	60000
#endif

/* comment pragma types */
#define COMMENT_END			0
#define COMMENT_COMPILER	1
#define COMMENT_DATE		2
#define COMMENT_EXESTR		3
#define COMMENT_LIB			4
#define COMMENT_TIMESTAMP	5
#define COMMENT_USER		6
#define COMMENT_LITERAL		7
#define COMMENT_EXPDEF		8
#define COMMENT_NOLIB		9	/* tell the linker: do NOT search this lib by default */
#define COMMENT_LINKER		10	/* arbitrary option strings passed to the linker */

#define KY_CURRENTLOCATION	1		/* reserved symbol key for "$" */

/* predefined segment keys */
#define KY_DATA 	2
#define KY_CONST	3
#define KY_BSS		4
#define KY_TEXT 	5
#define KY_STACK	6
#define KY_CLCRC	6 		// YES 6 just like KY_STACK, OK KY_STACK is never output (not a real segment) - sps
#define KY_ATEXT	7
#define KY_SYMBOLS  8
#define KY_TLS		9		// Used by P2 for TLS data

#define KY_LOCALSIZE	10	// Reserved symbol key for __FRAME_SIZE in inline asembler
#define KY_FARDATA		11  // Currently Wings specific. Specifies data which cannot be got to using 
							// 16-bit offset addressing,

#define KY_FARBSS	12		// Bss for the FARDATA segment above. 
							// Not used by FE just put in here to avoid key 
							// collision with the ALAR backend.	
#define KY_PDATA        13  // PowerPC (both)
#define KY_RELDATA      14  // PowerNT Relative Data?
#define KY_XDATA        15  // PowerNT exceptions

#if _VC_VER >= 300
#define KY_STARTUSERKEYS	16
#else
#define KY_STARTUSERKEYS	100
#endif

/* segment types */
#define SEGT_TEXT		0
#define SEGT_DATA		1
#define SEGT_ENDTEXT	2
#define SEGT_BSS		3
#define SEGT_CONST		4
#define SEGT_FARCONST	5
#define SEGT_TLS		6
#define SEGT_FARDATA	7
#define SEGT_UNKNOWN	8

#define SEGT_ISTEXT(a)		((a) == SEGT_TEXT)
#define SEGT_ISDATA(a)		((a) == SEGT_DATA)
#define SEGT_ISENDTEXT(a)	((a) == SEGT_ENDTEXT)
#define SEGT_ISBSS(a)		((a) == SEGT_BSS)
#define SEGT_ISCONST(a)		((a) == SEGT_CONST)
#define SEGT_ISFARCONST(a)	((a) == SEGT_FARCONST)
#define SEGT_ISTLS(a)		((a) == SEGT_TLS)
#define SEGT_ISFARDATA		((a) == SEGT_FARDATA)
#define SEGT_ISUNKNOWN(a)	((a) == SEGT_UNKNOWN)

#define SEGT_ISANYTEXT(a)	(SEGT_ISTEXT(a) || SEGT_ISENDTEXT(a))


/* thunk numbers */
#define THNK_ADJUSTOR	1
#define THNK_VCALL		2
#define THNK_VCALL16    2
#define THNK_VCALL32    3
#define THNK_VTDISP_ADJUSTER    4

// extended entry attribute descriptor

typedef char ExtEntryAttr_t;	// M00C9IL: This becomes __int16 for C9

#define EE_INLINE		0x80	// user explicit inline function - p2 read only
#define EE_EXPAND		0x40	// candidate for expansion
#define EE_INSTANTIATE	0x20	// function must be instantiated - p2 read only
#define EE_EXPANDING 	0x10	// currently being expanded
#define EE_EXPLICIT		0x08	// if comdat func, no anonymous allocation
#define EE_REFERENCED	0x04	// function was ref'd
#define EE_CGEN			0x02	// compiler generated function
#define EE_ILINPCH		0x01	// il can be found in the pch file

// New extended attributes, under C9IL only

#define EE_HASEHUNWIND	0x0100	// Function has EH unwind semantics
#define EE_HASEHTRY		0x0200	// Function has try/catch
#define EE_HASEHSTATES	0x0400	// Destructors are specified using OPnewstate / OPretstate
#if _VC_VER >= 300
#define EE_CHANGED		0x0800  // il has changed - for incremental
#endif


#define AUTOINLINETHRESHOLD		175

//
// EH State Attributes: (a dword so there's lots of room to grow)
//	(on OPnewstate)
//

#define EHA_FIXED		0x00000001	// Fixed state (mustn't be reordered)
#define EHA_MBARG		0x00000002	// Special state associated with IV_MBARG
#define EHA_TEMPORARY	0x00000003	// Construction of statement life temp
#define EHA_RETURN		0x00000004	// Construction of Return Expression

#define EHA_CLASS_MASK	0x00000007	// Mask for the above

#define EHA_ISFORMAL	0x00000008	// This State is ass. with a formal arg
#define EHA_NOUNWIND	0x00000010	// This Object needs no unwinding
#define EHA_MUSTPOP		0x00000020	// This State needs to be popped
#define EHA_CONDITIONAL	0x00000040	// This State is executed conditionally
#define EHA_ISDEAD		0x00000080	// This opcode is just cleaning up
#define EHA_CATCHRET	0x00000100	// This pushstate is used to exit a Catch block

#if CC_MACTRAP

#define	MacReg__None			0
#define	MacReg__D0				1
#define	MacReg__D1				2
#define	MacReg__D2				3
#define	MacReg__A0				4
#define	MacReg__A1				5
#define	MacReg__UpperBound		6

#define	MacTrapRegisterMask		7
#define	MacTrapBitsPerReg		3

#define	MacTrapReturn			0							 
#define	MacTrapParam1			1
#define	MacTrapParam2			2
#define	MacTrapParam3			3
#define	MacTrapParam4			4
#define	MacTrapParam5			5

#define	MacTrapEncodedMask		000777777UL
#define	MacTrapToil(v)			((v) & MacTrapEncodedMask)

#define	MacTrapError			001000000UL
#define	MacTrapVoidVoid			010000000UL
//
//  MacTrapEncodeReg(MacTrapParam3, MacReg__D2)
//	MacTrapDecodeReg(VarHoldingEncoding, MacTrapReturn)
//  (internal helpers for below)
//
#define	MacTrapEncodeReg(p, r)	(((r) << (MacTrapBitsPerReg * (p))))
#define	MacTrapDecodeReg(v, p)	(((v) >> (MacTrapBitsPerReg * (p))))
//
//  ClearMacTrap(VarHoldingEncodings, MacTrapParam4)
//  SetMacTrap(VarHoldingEncoding, MacTrapParam5, MacReg__A1)
//  GetMacTrap(VarHoldingEncoding, MacTrapParam3)
//
#define	ClearMacTrap(v, p)		((v) &= (~(MacTrapEncodeReg(p, MacTrapRegisterMask))))
#define	SetMacTrap(v, p, r)		(ClearMacTrap(v, p), (v) |= (MacTrapEncodeReg(p, r)))
#define	GetMacTrap(v, p)		(MacTrapDecodeReg(v, p) & MacTrapRegisterMask)
#endif
