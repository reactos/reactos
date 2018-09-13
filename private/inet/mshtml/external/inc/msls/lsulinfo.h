#ifndef LSULINFO_DEFINED
#define LSULINFO_DEFINED

#include "lsdefs.h"
#include "plsulinf.h"

/* 
 * Both Offsets are relative to the local baseline and positive "down" (towards blank page), 
 * so in horizontal Latin case dvpFirstUnderlineOffset usually is bigger than zero. 
 *
 * dvpUnderlineOrigin points to UnderlineOrigin - the place where (main part of the) letter ends and 
 * area for underlining begins. For Latin letters it is the Latin baseline. UnderlineOrigin shows if
 * one run of two runs is higher; runs with the same UnderlineOrigin can have their underlines averaged.
 *
 * dvpFirstUnderlineOffset points to the beginning of the closest to the UnderlineOrigin underline.
 * You have "underlining from above" case if dvpUnderlineOrigin > dvpFirstUnderlineOffset.
 *
 * Everything else should be positive. Instead of dvpSecondUnderlineOffset of the previous version dvpGap 
 * is used. Second underline is further away from the UnderlineOrigin than first underline, so
 * dvpSecondUnderlineOffset = dvpFirstUnderlineOffset + dvpFirstUnderlineSize + dvpGap in normal case.
 * In "underlining from above" case there will be minuses instead of pluses.
 *
 * Main merging rules: 
 *
 * LS will not merge runs with different kulbase or different cNumberOfLines.
 * LS will not merge runs with different negative dvpPos (subscripts)
 * LS will not merge subscripts with superscripts or baseline runs
 * LS will not merge "underlined above" run with "underlined below" run.
 *
 * If merging is possible: 
 * Runs with the same UnderlineOrigin are averaged.
 * If UnderlineOrigins are different, the run with higher UnderlineOrigin takes metrics from neighbor.
 */

struct lsulinfo
{
    UINT  kulbase;						/* base kind of underline */
    DWORD cNumberOfLines;				/* number of lines: possible values 1,2*/

	long dvpUnderlineOriginOffset;		/* UnderlineOrigin decides which run is higher */
	long dvpFirstUnderlineOffset;		/* offset for start of the (first) underline */
	long dvpFirstUnderlineSize;			/* width of the (first) underline */
	
	long dvpGapBetweenLines;			/* If NumberOfLines != 2, dvpGapBetweenLines */
	long dvpSecondUnderlineSize;		/* 	and dvpSecondUnderlineSize are ignored. */
	
};
typedef struct lsulinfo LSULINFO;


#endif /* !LSULINFO_DEFINED */

