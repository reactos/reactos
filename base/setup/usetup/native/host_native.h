#ifndef _HOST_NATIVE_H_
#define _HOST_NATIVE_H_

#include "usetup.h"

typedef struct
{
	PWCHAR Source;
	PWCHAR Target;
} *PFILEPATHS_W;

#define SetupInitDefaultQueueCallback(a) NULL
#define SetupDefaultQueueCallbackW(a, b, c, d) TRUE
#define SetupTermDefaultQueueCallback(a)

#endif /* _HOST_NATIVE_H_ */
