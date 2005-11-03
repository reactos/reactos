#include "syshdrs.h"

#if !defined(NO_UNIX_DOMAIN_SOCKETS) && !defined(NO_SIGNALS)

extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;

int
UAcceptA(int sfd, struct sockaddr_un *const addr, int *ualen, int tlen)
{
	int result;
	vsio_sigproc_t sigalrm, sigpipe;

	if (tlen < 0) {
		errno = 0;
		for (;;) {
			*ualen = (int) sizeof(struct sockaddr_un);
			result = accept(sfd, (struct sockaddr *) addr, (int *) ualen);
			if ((result >= 0) || (errno != EINTR))
				return (result);
		}
	}

	if (SSetjmp(gNetTimeoutJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		errno = ETIMEDOUT;
		return (kTimeoutErr);
	}

	sigalrm = (vsio_sigproc_t) SSignal(SIGALRM, SIOHandler);
	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIG_IGN);
	alarm((unsigned int) tlen);

	errno = 0;
	do {
		*ualen = (int) sizeof(struct sockaddr_un);
		result = accept(sfd, (struct sockaddr *) addr, (int *) ualen);
	} while ((result < 0) && (errno == EINTR));

	alarm(0);
	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
	return (result);
}	/* UAcceptA */

#endif

