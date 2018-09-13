/*	File: D:\WACKER\htrn_jis\htrn_jis.hh (Created: 24-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:06p $
 */

/*
 * The following are the currently defined modes for input and output
 * character translation
 */
#define	PASS_THRU_MODE				1
#define	JIS_TO_SHIFT_MODE			2
#define	SHIFT_TO_JIS_MODE			3
#define	SHIFT_TO_EUC_MODE			4
#define	EUC_TO_SHIFT_MODE			5

/*
 * The following structures are used to translate the character stream
 * between Shift-JIS and JIS.
 */
struct stShiftToJis
	{
	int nInTwoByteMode;			/* already sent a mode shift sequence */
	int nLeadByteSeen;			/* previous char was a lead byte */
	int nHalfWidthKanaSeen;		/* previous char was a half width katakana */
	TCHAR chPrev;				/* the previous char */
	};

struct stJisToShift
	{
	int nInTwoByteMode;			/* already seen a mode shift sequence */
	int nSeenFirstCharacter;	/* already picked up one character */
	int nHalfWidthKanaSeen;		/* previous char was a half width katakana */
	TCHAR chPrev;				/* the character we picked up */
	int nSeenEscape;			/* we are collecting an escape sequence */
	int nEscSeqCount;			/* how many characters have we collected */
	TCHAR acBuffer[8];			/* if we exceed this, then we are lost */
	};

union uConvert
	{
	struct stShiftToJis stSTJ;
	struct stJisToShift stJTS;
	};

/*
 * The following in the internal data structure used to maintain state
 * information about the data stream and the translation mode
 */
struct stInternalCharacterTranslation
	{
	HSESSION hSession;		/* Needed by the load and dialog functions */
	int nInputMode;
	union uConvert uIn;		/* State information for input conversion */
	int nOutputMode;
	union uConvert uOut;	/* State information for output conversion */
	};

typedef	struct stInternalCharacterTranslation stICT;
typedef stICT *pstICT;

/*
 * The following macros come from the book "Understanding Japanese
 * Information Processing" by Ken Lunde.
 */

#define	SJIS1(A)	(((A >= 129) && (A <= 159)) || ((A >= 224) && (A <= 239)))
#define	SJIS2(A)	((A >= 64) && (A <= 252))
#define	HANKATA(A)	((A >= 161) && (A <= 223))
#define	ISEUC(A)	((A >= 161) && (A <= 254))

#define	ISMARU(A)	((A >= 202) && (A <= 206))
#define	ISNIGORI(A)	(((A >= 182) && (A <= 196)) || ((A >= 202) && (A <= 206)) || (A == 179))

