#pragma once

#include "usetup.h"

typedef struct
{
	PWCHAR Source;
	PWCHAR Target;
} *PFILEPATHS_W;

#define SetupInitDefaultQueueCallback(a) NULL
#define SetupDefaultQueueCallbackW(a, b, c, d) TRUE
#define SetupTermDefaultQueueCallback(a)
