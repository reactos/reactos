#include "syshdrs.h"

#if !defined(NO_UNIX_DOMAIN_SOCKETS) && !defined(NO_SIGNALS)

extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;

int
URecvfrom(int sfd, char *const buf, size_t size, int fl, struct sockaddr_un *const fromAddr, int *ualen, int tlen)
{
	int nread, tleft;
	vsio_sigproc_t sigalrm, sigpipe;
	time_t done, now;

	if (SSetjmp(gNetTimeoutJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		errno = ETIMEDOUT;
		return (kTimeoutErr);
	}

	if (SSetjmp(gPipeJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		errno = EPIPE;
		return (kBrokenPipeErr);
	}

	sigalrm = (vsio_sigproc_t) SSignal(SIGALRM, SIOHandler);
	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIOHandler);

	time(&now);
	done = now + tlen;
	tleft = (int) (done - now);
	forever {
		*ualen = sizeof(struct sockaddr_un);
		(void) alarm((unsigned int) tleft);
		nread = recvfrom(sfd, buf, size, fl,
			(struct sockaddr *) fromAddr, ualen);
		(void) alarm(0);
		if (nread >= 0)
			break;
		if (errno != EINTR)
			break;		/* Fatal error. */
		errno = 0;
		time(&now);
		tleft = (int) (done - now);
		if (tleft < 1) {
			nread = kTimeoutErr;
			errno = ETIMEDOUT;
			break;
		}
	}

	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);

	return (nread);
}	/* URecvfrom */

#endif
