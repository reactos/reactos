/* SCCSWHAT( "@(#)labdat.h	3.2 89/12/06 13:46:15	" ) */
/*	labdat.h - translation tables and code for internal names.
 *
 *	DESCRIPTION
 *		This file contains the translation tables and code necessary 
 *		to generate internal names in the CMERGE compilers. This module
 *		is used to reduce the size of the SYM_IL and SIL. The DIL 
 *		(debug IL) emitter uses this module to expand the short forms
 *		of internal names into character strings.
 *
 *		An underscore must separate %d from %s because if %s is a numeric
 *		string, then values can be repeated. For example %d = 10, %s = 120
 *		gives 10_120. %d = 101, %s = 20 gives 101_20. Both would give 10120
 *		without the underscore.
 *
 *		Note that NA_USER MUST be 0. The table can not be conditionally
 *		compiled unless the script that makes the enumerations knows about
 *		#ifdefs.
 *
 *	AUTHOR
 *		Gordon Whitten			November 29, 1983.
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

LABDAT(NA_USER,				"_%d_%s")		/* User defined labels. */

LABDAT(NA_CASE,				".C%d_%s")		/* Pascal CASE. */
LABDAT(NA_CCONST,			".CC%d_%s")
LABDAT(NA_CEXIT,			".CE%d_%s")
LABDAT(NA_COTHERWISE,		".CO%d_%s")

LABDAT(NA_DO,				".D%d_%s")		/* C do. */
LABDAT(NA_DBREAK,			".DB%d_%s")
LABDAT(NA_DCONTINUE,		".DC%d_%s")

LABDAT(NA_FOR,				".F%d_%s")		/* C for. */
LABDAT(NA_FORDOWN,			".FD%d_%s")		/* Pascal FOR DOWNTO .. */
LABDAT(NA_FORUP,			".FU%d_%s")		/* Pascal FOR .. TO .. */
LABDAT(NA_FBREAK,			".FB%d_%s")
LABDAT(NA_FCONTINUE,		".FC%d_%s")
LABDAT(NA_FCYCLE,			".FC%d_%s")

LABDAT(NA_REPEAT,			".R%d_%s")		/* Pascal REPEAT. */
LABDAT(NA_RBREAK,			".RB%d_%s")
LABDAT(NA_RCYCLE,			".RC%d_%s")

LABDAT(NA_SWITCH,			".S%d_%s")		/* C switch. */
LABDAT(NA_SBREAK,			".SB%d_%s")
LABDAT(NA_SCASE,			".SC%d_%s")
LABDAT(NA_SDEFAULT,			".SD%d_%s")

LABDAT(NA_WHILE,			".W%d_%s")		/* Pascal, C while. */
LABDAT(NA_WBREAK,			".WB%d_%s")
LABDAT(NA_WCONTINUE,		".WC%d_%s")
LABDAT(NA_WCYCLE,			".WC%d_%s")

LABDAT(NA_ALIAS,			"%s")			/* Pascal aliasing. */
LABDAT(NA_BREAK,			".B%d_%s")		/* Pascal, C break. */
LABDAT(NA_CONTINUE,			".CO%d_%s")		/* C continue. */
LABDAT(NA_DEFAULT,			".DE%d_%s")		/* C default. */
LABDAT(NA_ELSE,				".E%d_%s")		/* Pascal ELSE */
LABDAT(NA_ENDIF,			".I%d_%s")		/* Pascal, C endif. */
LABDAT(NA_EXIT,				".EX%d_%s")		/* Procedure exit point. */
LABDAT(NA_LABEL,			".L%d_%s")		/* Local labels. */
LABDAT(NA_PARTIAL,			".PA%d_%s")		/* Array address partial. */
LABDAT(NA_POCKET,			".PO%d_%s")		/* Pocket code. */
LABDAT(NA_START,			".ST%d_%s")		/* Procedure beginning. */
LABDAT(NA_STATIC,			".S%d_%s")		/* Static variables. */
LABDAT(NA_STRING,			".SG%d_%s")		/* String variables. */
LABDAT(NA_TEMP,				".T%d_%s")		/* P2 Temporaries. */

LABDAT(NA_DOEXP3,			"_V%d_%s_DO3EX")	/* third DO loop expr */
LABDAT(NA_DOLOOP,			"_V%d_%s_DO_CNT")	/* DO loop counter */
LABDAT(NA_SP_STRING,		"_V%d_%s_STOP_PAWS")	/* stop/pause string */
LABDAT(NA_ICOERCE,			"_V%d_%s_COINTRIN")	/* coerse intrin func */
LABDAT(NA_IFUNC,			"_V%d_%s_INTRINFN")	/* intrin func name */
LABDAT(NA_FMTSYM,			"_V%d_%s_FMTNAME")	/* FORMAT name */
LABDAT(NA_FMT_VAR,			"_V%d_%s_FMTIVAR")	/* FORMAT int var */
LABDAT(NA_FMT_INTERNAL,		"_V%d_%s_FMT_INTERN")	/* intern FORMAT */
LABDAT(NA_FMT_DESC,			"_V%d_%s_IO_DESC")	/* I/O descriptor */
LABDAT(NA_FMT_BYREF,		"_V%d_%s_BYREF")	/* tmp for ref val */
LABDAT(NA_IOEX,				"_V%d_%s_IOEX")		/* I/O exception */
LABDAT(NA_IOCHARLEN,		"_V%d_%s_CHARLEN")	/* char len i/o */
LABDAT(NA_SIDE_EFFECT,		"_V%d_%s_SIDEFF")	/* side effect expr */
LABDAT(NA_NEW_ACTUAL,		"_V%d_%s_NEWACT")	/* new actual parm */
LABDAT(NA_TEMP_STRING,		"_V%d_%s_STRVAR")	/* string var */

/*	NOTE:code in FORTRAN/fdbil.c depends upon these next two names.
 *	see mk_iname for details. Do not change these names.
 */
LABDAT(NA_LOWER,			"_V%d_%s_LBND")		/* bound, varlen arrs */
LABDAT(NA_UPPER,			"_V%d_%s_UBND")		/* bound, var len arr */

LABDAT(NA_SIZE,				"_V%d_%s_SIZE")		/* size, var len arr */
LABDAT(NA_BIAS,				"_V%d_%s_BIAS")		/* bias, var len arr */
LABDAT(NA_CONT_VBA,			"_V%d_%s_VBACONT")	/* cont lab, arr init */
LABDAT(NA_RETURN_LABEL,		"_V%d_%s_RETURN")	/* return lab alt ret */
LABDAT(NA_L_FORMAT,			".L%d_%s_FORMAT")	/* format label */
LABDAT(NA_L_EAGOTO,			".L%d_%s_ENDAGOTO")	/* end of assign goto */
LABDAT(NA_L_DOEXIT,			".L%d_%s_DOEXIT")	/* do loop exit */
LABDAT(NA_L_DOTOP,			".L%d_%s_DOTOP")	/* top of do loop */
LABDAT(NA_L_AGOTOSW,		".L%d_%s_SW_AGOTO")	/* assign goto switch */
LABDAT(NA_L_CGOTOSW,		".L%d_%s_SW_CGOTO")	/* comp goto switch */
LABDAT(NA_L_BLOCK_IF,		".L%d_%s_BLOCKIF")	/* block if */
LABDAT(NA_L_LOG_IF,			".L%d_%s_LOGIF")	/* logical if */
LABDAT(NA_L_VBAINIT,		".L%d_%s_VBAINIT")	/* var bound arr init*/
LABDAT(NA_L_AND,			".L%d_%s_ANDDECOMP")	/* and decomp */
LABDAT(NA_L_OR,				".L%d_%s_ORDECOMP")	/* or decomp */

LABDAT(NA_UNWIND,			".U%d_%s_UNWIND")	/* C++ Forward Goto Unwind Alias Label */
LABDAT(NA_OUTOFTRY,			".T%d_%s_OUTOFTRY")	/* C++ Exception Handling */

#if 0
/*  VERSION needs to be NA_VERSION  */
LABDAT(VERSION,			"_V%d_%s_VERSION")		/* version string */
#endif
