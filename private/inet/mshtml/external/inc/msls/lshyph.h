#ifndef LSHYPH_DEFINED
#define LSHYPH_DEFINED

#include "lsdefs.h"
#include "plshyph.h"

struct lshyph							/* Output of pfnHyphenate callback */
{
	UINT kysr;							/* Kind of Ysr - see "lskysr.h" */
	LSCP cpYsr;							/* cp value of YSR */
	WCHAR wchYsr;						/* YSR char code  */
};

#endif /* !LSHYPH_DEFINED */
