#include "syshdrs.h"

#if !defined(NO_UNIX_DOMAIN_SOCKETS) && !defined(NO_SIGNALS)

extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;

int
UConnect(int sfd, const struct sockaddr_un *const addr, int ualen, int tlen)
{
	int result;
	vsio_sigproc_t sigalrm;

	if (SSetjmp(gNetTimeoutJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		errno = ETIMEDOUT;
		return (kTimeoutErr);
	}

	sigalrm = (vsio_sigproc_t) SSignal(SIGALRM, SIOHandler);
	alarm((unsigned int) tlen);

	errno = 0;
	do {
		result = connect(sfd, (struct sockaddr *) addr, ualen);
	} while ((result < 0) && (errno == EINTR));

	alarm(0);
	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
	return (result);
}	/* UConnect */

#endif

