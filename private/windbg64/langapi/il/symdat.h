/* SCCSWHAT( "@(#)symdat.h	3.4 89/12/06 13:48:27	" ) */
/*
**
** IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT 
** if any changes are made to this file, the ILID_P1 constant in
** common\ilvers.h should be changed to reflect the date of the change
**
*/

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

/*
 * This is the dope table for the symbol stream. The attributes are as
 * follows :
 *
 *	A (01) - allocation length					(long)
 *	a (02) - attributes							(long)
 *	B (03) - symbol record type 	 			(char)
 *	e (04) - element size						(long)
 *	i (06) - type index							(short)
 *	k (05) - the key						  	(key)
 *	L (07) - lexical level						(char)
 *	n (010)- the template for name generation	(char)
 * 	N (011)- Real name ( for static vars )		(text()-string\0)
 *	R (012)- backward/forward for labels		(char)
 *	s (013)- static nesting level (absolute)	(char)
 *	S (014)- allocation base					(char)
 *	t (015)- primitive type (of object)			(compacted short)
 *	U (016)- label use count					(short)
 * 	c (017)- segment class name
 *	V (020)- visibility class					(char)
 *	v (021)- entry visibility class				(char)
 *	x (022)- segment key						(key)
 *	o (023)- offset of a comequ/alias			(short)
 *	E (024)- val of a constant/cconstant		(short,<short> bytes)
 *	Z (025)- alias key for comequ/alias			(key)
 *	K (026)- key of containing entry record		(key)
 *  g (027)- segment type 						(char)
 *  ? (030)- file offset ( both ex and sy )				(long + long)
 *  ? (031)- number of ex nodes					(short)
 *  ? (032)- ext entry flag						(long)
 *  ? (033)- loop data							(short,<short> bytes)
 *  w (034)- key of weak extern sym 2 			(key)
 *  ? (035)- product id of header 				(short)
 *  ? (036)- p1 il id of header 				(short)
 *  ? (040)- translator 						(char)
 *  ? (041)- trailer	 						(multi-byte)
 *  ? (042)- filename 							(key + name)
 *  ? (043)- num parms                          (short)
				bit 0x01 (SET=DATA, CLEAR=CODE)
 *	? (044)- byte stream (Centaur Backend)		(long, <long> bytes)
 *  ? (045)- max p1 key 						(long)
 *  ? (046)- ilmod name							(long)
 *	? (050)- function start line number			(long)
 *	? (051)- IL version number					(short)
NOTE: there is a P2 dependency on Symdat_BKkAni: the 'K' information
	  must preceed the 'n' information on any SSR record.
*/

#define	Symdat_b ""
#define	Symdat_B "\03"
#define	Symdat_BLknNtsSVAai "\03\07\05\010\011\015\013\014\020\01\02\06"
#define	Symdat_BLknNtsSVAaie "\03\07\05\010\011\015\013\014\020\01\02\06\04"
#define	Symdat_BLknNUR "\03\07\05\010\011\016\012"
#define	Symdat_BknNtvAai "\03\05\010\011\015\021\01\02\06"
#define	Symdat_BKkAni "\03\026\05\01\010\06"
#define Symdat_BLkNtsAVoiZe "\03\07\05\011\015\013\01\020\023\06\025\04"
#define Symdat_BLkNitsAE "\03\07\05\011\06\015\013\01\024"
#define	Symdat_BkNgc "\03\05\011\027\017"
#define	Symdat_BLkNsi	"\03\07\05\011\013\06"
#define Symdat_BL "\03\07"
#define	Symdat_BknNtvAaiw "\03\05\010\011\015\021\01\02\06\034"

	IL_DAT(SSR_UNDEFINED,	Symdat_B,				SIZEOF2(SYMBLOCK))
	IL_DAT(SSR_NAME,		Symdat_BLknNtsSVAai,	SIZEOF2(SYMNAME))
	IL_DAT(SSR_ARRAY,		Symdat_BLknNtsSVAaie,	SIZEOF2(SYMARRAY))
	IL_DAT(SSR_LABEL,		Symdat_BLknNUR,			SIZEOF2(SYMLABEL))
	IL_DAT(SSR_ENTRY,		Symdat_BknNtvAai,		SIZEOF2(SYMENTRY))
	IL_DAT(SSR_RETURN,		Symdat_BKkAni,			SIZEOF2(SYMRETURN))
	IL_DAT(SSR_BLOCK,		Symdat_B,				SIZEOF2(SYMBLOCK))
	IL_DAT(SSR_COMEQU,		Symdat_BLkNtsAVoiZe,	SIZEOF2(SYMARRAY))
	IL_DAT(SSR_CONSTANT,	Symdat_BLkNitsAE,		SIZEOF2(SYMCONSTANT))
	IL_DAT(SSR_SEG,			Symdat_BkNgc,			SIZEOF2(SYMSEG))
	IL_DAT(SSR_TRAILER,		"\03\041",				0)
    IL_DAT(SSR_TYPEDEF,		Symdat_BLkNsi,			SIZEOF2(SYMNAME))
	IL_DAT(SSR_INFO,		Symdat_b,				SIZEOF2(SYMBLOCK))
	IL_DAT(SSR_START,		Symdat_BL,				SIZEOF2(SYMBLOCK))
#if _VC_VER >= 300
    IL_DAT(SSR_EXTENTRY,	"\03\05\010\011\015\021\01\02\06\030\031\032\043\050\051", SIZEOF2(SYMEXTENT)) /* C10IL */
#else
    IL_DAT(SSR_EXTENTRY,	"\03\05\010\011\015\021\01\02\06\030\031\032\043", SIZEOF2(SYMEXTENT))
#endif
	IL_DAT(SSR_LOOP,		"\03\033",				0)
	IL_DAT(SSR_WEAKENTRY,	Symdat_BknNtvAaiw,		SIZEOF2(SYMWEAKENTRY))
    // numerical order of SSR_HEADER must be maintained in order for version 
    // checking to work    
	IL_DAT(SSR_HEADER,		"\03\035\036\045\040",	0)
	IL_DAT(SSR_FILENAME,	"\03\042",				0)
	//For Centaur Backend: P1 shoudn't emit this!!!
	IL_DAT(SSR_UPTYPE,		"\03\044",	0)
	IL_DAT(SSR_RESCOPE,		"\03\05\025\026",	0)
	IL_DAT(SSR_NESTEDENTRY,		"\03\05\026",	0)
#if _VC_VER >= 300
	IL_DAT(SSR_ILMOD,		"\03\046",			0)
	IL_DAT(SSR_PDB_IL,		"\03\047",			0)
#endif
