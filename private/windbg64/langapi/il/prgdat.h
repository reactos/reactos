/* SCCSWHAT( "@(#)prgdat.h	3.3 89/12/06 13:47:43	" ) */
/*
**
** IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT 
** if any changes are made to this file, the ILID_P1 constant in
** common\ilvers.h should be changed to reflect the date of the change
**
 * Definitions of PRAGMA attributes.  These attributes are used by P1 to
 * write the correct information and by P2 to know what kind of information
 * to read when an OPpragma node is read in the EXP_IL stream.  These are
 * triggered by the pragma number contained in the OPpragma node.  Those
 * pragmas with a 1 in the third field are passed through from p2 to p3. The
 * attributes are defined as follows:
 *
 *		'f' (  ) - (short) value used to represent FALSE
 *		'k' (  ) - (int) symbol table key for variable to be kept in global register
 *		'l' (01) - (long) line number
 *		'm' (02) - (char string) module name (zero terminated - of length 'n')
 *		'n' (  ) - (char) number of characters (including zero terminator) in string
 *		'p' (  ) - (short) profiler symbol key
 *		's' (03) - (char string) file name (zero terminated - of length 'n')
 *		't' (  ) - (short) value used to represent TRUE
 *		'y' (04) - (char) 0 (stack checking off) or 1 (stack checking on)
 *		'K'	(  ) - (int) Symbol containing the file name in INITIL.
 *		'I' (  ) - (char) Default integer size.
 *		'd' (  ) - not a real attribute, just a bloody hack. turn on Debug
 *		'C' (  ) - not a real attribute, just a bloody hack. turn on Do66
 *		'o' (  ) - not a real attribute, just a bloody hack. turn off Debug
 *		'F' (  ) - same.
 *		'A' (  ) - same.
 *		'M' (  ) - same.
 *		'L' (05) - Translator used.
 *		'O' (06) - Loop Optimizations on/off flag.
 *		'?' (07) - Switch Checking on/off flag.
 *		'?' (010)- Segment name for load ds.
 *		'?' (011)- PLMN flag.
 *		'?' (012)- PLMF flag.
 *		'?' (013)- Output a comment record
 *		'?' (014)- Inline assembler statement
 *		'?' (015)- Inline assembler optimization flag
 *		'?' (016)- Inline assembler alignment flag
 *		'?' (017)- same.
 *		'?' (020)- same.
 *		'?' (021)- Optimize user pragma.
 *		'?' (022)- obsolete - assert if encountered
 *		'?' (023)- fortran - not support for inline function
 *		'?' (024)- pragma status
 *		'?' (025)- inline pragma status
 *		'?' (026)- limits pragma, ie max key
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

IL_DAT(PR_UNDEFINED,	"\022",		0)	/* bad pragma */
IL_DAT(PR_LINENUMBER,	"\01",	0)	/* current file line number */
IL_DAT(PR_FILENAME,		"\03",	0)	/* working file name */
IL_DAT(PR_DEBUG,		"\03",	0)	/* $DEBUG. */
IL_DAT(PR_NODEBUG,		0,		0)	/* $NODEBUG. */
IL_DAT(PR_DOSIX,		"\023",	0)	/* $DO66. */
IL_DAT(PR_ISIZE,		"\023",	0)	/* $STORAGE:n. */
IL_DAT(PR_REG,			0,		0)	/* avail. */
IL_DAT(PR_DEREG,		0,		0)	/* avail. */
IL_DAT(PR_PTRCHK,		0,		0)	/* avail. */
IL_DAT(PR_SWCHK,		"\07",	0)	/* avail. */
IL_DAT(PR_COMMENT,		"\013",	0)	/* output a comment record. */
IL_DAT(PR_DECMATH,		"\023",	0)	/* $DECMATH. */
IL_DAT(PR_FLOAT,		"\023",	0)	/* $FLOATCALLS. */
IL_DAT(PR_NOFLOAT,		"\023",	0)	/* $NOFLOATCALLS. */
IL_DAT(PR_BLKSTART,		0,		1)	/* start of C "block" */
IL_DAT(PR_BLKEND,		0,		1)	/* end of C "block" */
IL_DAT(PR_DBSTART,		0,		1)	/* start of debuggable code */
IL_DAT(PR_DBEND,		0,		1)	/* end of debuggable code */
IL_DAT(PR_DATASEG,		"\010", 0)	/* segment name for load ds. OBSOLETE*/
IL_DAT(PR_PLMN,			"\011",	0)	/* plmn. */
IL_DAT(PR_PLMF,			"\012",	0)	/* plmn. */
IL_DAT(PR_ASM,			"\014", 0)	/* inline assembler statement */
IL_DAT(PR_ASMOPT,		"\015", 0)	/* inline assembler optimization dir */
IL_DAT(PR_ASMALIGN,		"\016", 0)	/* inline assembler alig directive */
IL_DAT(PR_ASMBLOCKST,	0,		0)	/* inline assembler block start */
IL_DAT(PR_ASMBLOCKEND,	0,		0)	/* inline assembler block end */
IL_DAT(PR_REGVSTART,	0,		1)	/* start of reg alloc range */
IL_DAT(PR_REGVEND,		0,		1)	/* end of reg alloc range */
IL_DAT(PR_DSSTATE,	    0,		0)  /* DS == SS state control */
IL_DAT(PR_FUNPOP,		"b",	1)	/* # bytes popped by function */
IL_DAT(PR_PRAGSTAT,		"\024", 1)	/* pragma status */
IL_DAT(PR_INLINE,		"\025",	1)	// control inline expansion pragmas
IL_DAT(PR_ENTRYMSK,		"q",	1)	// entry mask
IL_DAT(PR_VTORDISP,		0, 		1)	// vtordisp en/disable
IL_DAT(PR_WARNING,		"\027", 0)	// pragma warning
#if _VC_VER >= 300
IL_DAT(PR_DELTALINE,	"\02",	0)	// relative line number (delta from previous PR_LINENUMBER or func)
#endif
IL_DAT(PR_LABEL,		"\030", 0)	// label to use for overflow checks (BASIC)
#if _VC_VER > 500
IL_DAT(PR_PROBABILITY,	"\031",	0)	// branch execution probabilty
#endif
#if _VC_VER > 500
IL_DAT(PR_NODEFAULT,	0,		0)	// switch statement has no default branch
#endif
IL_DAT(PR_MAX,			0,	0)
