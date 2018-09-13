#ifndef FMTIO_DEFINED
#define FMTIO_DEFINED

#include "lsdefs.h"
#include "pfmti.h"
#include "plsdnode.h"
#include "lsfgi.h"
#include "lsfrun.h"
#include "lstxm.h"

/* ------------------------------------------------------------------------ */

struct fmtin
{
	LSFGI lsfgi;
	LSFRUN lsfrun;
	PLSDNODE plsdnTop;
	LSTXM lstxmPres;
	LSTXM lstxmRef;
};

/* ------------------------------------------------------------------------ */


#endif /* !FMTIO_DEFINED */
