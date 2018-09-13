#ifndef LSTXM_DEFINED
#define LSTXM_DEFINED

#include "lsdefs.h"
#include "plstxm.h"
/*igorzv** good explanation according text flow issue is needed here /
/* A few words about the v-vector and the sign bit: 
 *
 * dvDescent is positive downwards.		
 * v is positive upwards during formatting.
 */

struct lstxm
{
	long dvAscent;
	long dvDescent;

	long dvMultiLineHeight;
	BOOL fMonospaced;
};
typedef struct lstxm LSTXM;


#endif /* !LSTXM_DEFINED */
