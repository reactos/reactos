#ifndef FMTRES_DEFINED
#define FMTRES_DEFINED

/* used in fmtio.h and lsfd.h */

enum fmtres							/* Why did the formatter return? */
{
	fmtrCompletedRun,				/* no problems */
	fmtrExceededMargin,				/* reached right margin */
	fmtrTab,						/* reached tab				  */
	fmtrStopped						
};

typedef enum fmtres FMTRES;

#endif /* !FMTRES_DEFINED */
