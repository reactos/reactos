#ifndef LSCHP_DEFINED
#define LSCHP_DEFINED

#include "lsdefs.h"
#include "plschp.h"

#define idObjTextChp	((WORD)~0)		/* idObj for FetchRun to use  */
										/*  when returning a text run */
										/*  (Internal id will differ.) */

/* LS expects that for GlyphBased runs the following flags are set to FALSE:
			fApplyKern
			fModWidthSpace
			fModWidthPairs
			fCompressTable
*/

struct lschp							/* Character properties */
{
	WORD idObj;							/* Object type */
	BYTE dcpMaxContext;

	BYTE EffectsFlags;

    /* Property flags */
	UINT fApplyKern : 1;
	UINT fModWidthOnRun:1;
	UINT fModWidthSpace:1;
	UINT fModWidthPairs:1;
	UINT fCompressOnRun:1;
	UINT fCompressSpace:1;
	UINT fCompressTable:1;
	UINT fExpandOnRun:1;
	UINT fExpandSpace:1;
	UINT fExpandTable:1;
	UINT fGlyphBased : 1;

	UINT pad1:5;

	UINT fInvisible : 1;
	UINT fUnderline : 1;				
	UINT fStrike : 1;
	UINT fShade : 1;				
	UINT fBorder : 1;				
	UINT fHyphen : 1;					/* Hyphenation opportunity (YSR info) */
	UINT fCheckForReplaceChar : 1;		/* Activate the replace char mechanizm for Yen	*/

	UINT pad2:9;
										/* for dvpPos values, */
										/*  pos => raised, neg => lowered, */
	long dvpPos;
};

typedef struct lschp LSCHP;

#endif /* !LSCHP_DEFINED */
