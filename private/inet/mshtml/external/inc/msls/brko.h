#ifndef BRKO_DEFINED
#define BRKO_DEFINED

#include "lsdefs.h"
#include "pbrko.h"
#include "objdim.h"
#include "posichnk.h"
#include "brkcond.h"

typedef struct brkout						/* break output */ 
{
	BOOL fSuccessful;						/* break result */
	BRKCOND brkcond;						/* iff !fSuccessful, recommendation on the other side */
	POSICHNK posichnk;
	OBJDIM objdim;
} BRKOUT;


#endif /* !BRKO_DEFINED                    */
