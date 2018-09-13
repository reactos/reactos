#ifndef OBJDIM_DEFINED
#define OBJDIM_DEFINED

#include "lsdefs.h"
#include "pobjdim.h"
#include "heights.h"

typedef struct objdim							/* Object dimensions */
{
	HEIGHTS heightsRef;	
	HEIGHTS heightsPres;	
	long dur;
} OBJDIM;

#endif /* !OBJDIM_DEFINED */
