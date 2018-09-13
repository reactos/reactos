/* SCCSWHAT( "@(#)initdat.h	3.2 89/12/06 13:45:43	" ) */
/* INITIL - Initialization Intermediate Language information table definitions
 *
**
** IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT 
** if any changes are made to this file, the ILID_P1 constant in
** common\ilvers.h should be changed to reflect the date of the change
**
 * ARGUMENTS
 *		None
 *
 * RETURNS
 *		None
 *
 * SIDE EFFECTS
 *		None
 *
 * DESCRIPTION
 *		This module contains the tables which define the interface between
 *		p1, p2, and p3 for initialized data.
 *
 *		The attributes indicate the following values
 *
 *		"d"		(short) key to symbol table entry for initialized symbol
 *		"f"		(long) offset to be added to destination symbol address
 *		"i"		(char) align data to i-byte boundary
 *		"l"		(short/chars) n characters, string of n chars
 *		"b"		built-in: n bytes, length at 2nd and 3rd byte
 *		"m"		(char) record type (always DATA)
 *		"n"		(long) # bytes to skip (not initialize) in initialized structure
 *		"o"		(long) offset from source symbol for start of initialization
 *		"p"		(char) primitive type (SIGNED/UNSIGNED INT or REAL)
 *		"r"		(short) repeat count (number of times to emit the data)
 *		"s"		(short) key to symbol table entry for source symbol
 *		"t"		(char) sub_record type
 *		"v"		integer/floating point constant (size implied by "z")
 *		"x"		(no value passed) indicates start of DATA sub_records 
 *		"z"		(char) size of primitive value (char, word, double, etc)
 *		"h"		expression-il in init file
 *		"g"		(char) language id (for dbil)
 *		"B"		(char) begin bit offset
 *		"N"		(char) size of bitfield
 *
 * AUTHOR
 *		Written by: Dave Weil
 *		Date: December 15, 1982
 *
 * MODIFICATIONS			(By)				(Date)
 *
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

IL_DAT(I_DATA,		"mdox",			0)
IL_DAT(I_OBJECT,	"tpzv",			SIZEOF2(OBJ_REC))
IL_DAT(I_SYMBOLIC,	"tsfz",			SIZEOF2(SYMB_REC))
IL_DAT(I_MULTIBYTE,	"tl",			SIZEOF2(MB_REC))
IL_DAT(I_ALIGN,		"ti",			SIZEOF2(ALIGN_REC))
IL_DAT(I_REPEAT,	"tr",			SIZEOF2(REPEAT_REC))
IL_DAT(I_ENDREPEAT,	"t",			SIZEOF2(ENDREPEAT_REC))
IL_DAT(I_ENDDATA,	"t",			SIZEOF2(ENDDATA_REC))
IL_DAT(I_SKIP,		"tn",			SIZEOF2(SKIP_REC))
IL_DAT(I_ENDBLOCK,	"td",			SIZEOF2(ENDBLOCK_REC))
IL_DAT(I_DESTTYPES,	"mox",			0)
IL_DAT(I_SRCTYPES,	"tb",			SIZEOF2(MB_REC))
IL_DAT(I_P1INFO,	"tg",			0)
IL_DAT(I_EXPIL,		"th",			0)
#if _VC_VER >= 300
IL_DAT(I_PDB_IL,	"te",			0)
IL_DAT(I_ENDSTREAM,	"t",			0)
#endif
IL_DAT(I_BITFIELD,	"tBN",			0)
IL_DAT(I_ENDBITFIELD,	"tpzv",		0)