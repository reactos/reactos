#ifndef LSDEVICE_DEFINED
#define LSDEVICE_DEFINED

enum lsdevice				/* Parameter for pfnGetRunTextMetrics callback */
{
	lsdevPres,
	lsdevReference
};

typedef enum lsdevice LSDEVICE;

#endif /* !LSDEVICE_DEFINED */
