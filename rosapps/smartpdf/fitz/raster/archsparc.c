/*
SPARC specific render optims live here
*/
#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

#ifdef HAVE_VIS

#endif

#if defined (ARCH_SPARC)
void
fz_accelerate(void)
{
#  ifdef HAVE_VIS
	if (fz_cpuflags & HAVE_VIS)
	{
	}
#  endif
}
#endif

