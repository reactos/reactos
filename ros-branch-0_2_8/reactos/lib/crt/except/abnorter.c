#include "precomp.h"
#include <stdio.h>


#ifdef __GNUC__

/*
 * @unimplemented
 */
int _abnormal_termination(void)
{
	printf("Abnormal Termination\n");
//	return AbnormalTermination();
        return 0;
}

#else
#endif
