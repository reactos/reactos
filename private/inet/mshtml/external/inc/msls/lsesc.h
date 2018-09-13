#ifndef LSESC_DEFINED
#define LSESC_DEFINED

/* Definition of Line Services escape characters.
 * Used for LsFetchDispatchEsc().
 */

#include "lsdefs.h"

typedef struct
{
	WCHAR wchFirst, wchLast;			/* Range of chars codes */
} LSESC;


#endif /* !LSESC_DEFINED */
