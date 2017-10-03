#define _sio_main_c_
#include "syshdrs.h"

int gLibSio_Uses_Me_To_Quiet_Variable_Unused_Warnings = 0;

#ifndef NO_SIGNALS

volatile Sjmp_buf gNetTimeoutJmp;
volatile Sjmp_buf gPipeJmp;

void
SIOHandler(int sigNum)
{
	if (sigNum == SIGPIPE)
		SLongjmp(gPipeJmp, 1);
	SLongjmp(gNetTimeoutJmp, 1);
}	/* SIOHandler */




void (*SSignal(int signum, void (*handler)(int)))(int)
{
#ifdef HAVE_SIGACTION
	struct sigaction sa, osa;

	(void) sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	if (signum == SIGALRM) {
#ifdef SA_INTERRUPT
		sa.sa_flags |= SA_INTERRUPT;
#endif
	} else {
#ifdef SA_RESTART
		sa.sa_flags |= SA_RESTART;
#endif
	}
	if (sigaction(signum, &sa, &osa) < 0)
		return (SIG_ERR);
	return (osa.sa_handler);
#else
	return SSignal(signum, handler);
#endif	/* HAVE_SIGACTION */
}

#endif	/* NO_SIGNALS */

/* eof main.c */
