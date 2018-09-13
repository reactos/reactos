#ifndef LSCONTXT_DEFINED
#define LSCONTXT_DEFINED

#include "lsdefs.h"
#include "lscbk.h"
#include "lstxtcfg.h"
#include "lsimeth.h"


typedef struct 
{
	DWORD version;						/* version number	*/
	DWORD cInstalledHandlers;
	const LSIMETHODS* pInstalledHandlers; /* Installed handlers */
	LSTXTCFG lstxtcfg;					/* Straight-text configuration data */
	POLS pols;							/* Client data for this context */
	LSCBK lscbk;						/* LineServices client callbacks */
	BOOL fDontReleaseRuns;				/* Optimization---don't call pfnReleaseRun */
} LSCONTEXTINFO;


LSERR WINAPI LsCreateContext(const LSCONTEXTINFO*, PLSC*);
LSERR WINAPI LsDestroyContext(PLSC);

#endif /* LSCONTXT_DEFINED */
