#include "syshdrs.h"

#ifndef NO_SIGNALS
extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;
#endif

#ifndef NO_SIGNALS

int
SSendtoByName(int sfd, const char *const buf, size_t size, int fl, const char *const toAddrStr, int tlen)
{
	int nwrote, tleft, result;
	vsio_sigproc_t sigalrm, sigpipe;
	time_t done, now;
	struct sockaddr_in toAddr;

	if ((result = AddrStrToAddr(toAddrStr, &toAddr, -1)) < 0) {
		return (result);
	}

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
		(void) alarm((unsigned int) tleft);
		nwrote = sendto(sfd, buf, size, fl,
				(struct sockaddr *) &toAddr,
				(int) sizeof(struct sockaddr_in));
		(void) alarm(0);
		if (nwrote >= 0)
			break;
		if (errno != EINTR)
			break;		/* Fatal error. */
		errno = 0;
		time(&now);
		tleft = (int) (done - now);
		if (tleft < 1) {
			nwrote = kTimeoutErr;
			errno = ETIMEDOUT;
			break;
		}
	}

	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);

	return (nwrote);
}	/* SSendtoByName */

#else

int
SSendtoByName(int sfd, const char *const buf, size_t size, int fl, const char *const toAddrStr, int tlen)
{
	int nwrote, tleft;
	time_t done, now;
	fd_set ss;
	struct timeval tv;
	int result;
	struct sockaddr_in toAddr;

	if ((result = AddrStrToAddr(toAddrStr, &toAddr, -1)) < 0) {
		return (result);
	}

	time(&now);
	done = now + tlen;
	nwrote = 0;
	forever {
		forever {
			if (now >= done) {
				errno = ETIMEDOUT;
				return (kTimeoutErr);
			}
			tleft = (int) (done - now);
			errno = 0;
			FD_ZERO(&ss);
			FD_SET(sfd, &ss);
			tv.tv_sec = tleft;
			tv.tv_usec = 0;
			result = select(sfd + 1, NULL, SELECT_TYPE_ARG234 &ss, NULL, SELECT_TYPE_ARG5 &tv);
			if (result == 1) {
				/* ready */
				break;
			} else if (result == 0) {
				/* timeout */		
				errno = ETIMEDOUT;
				return (kTimeoutErr);
			} else if (errno != EINTR) {
				return (-1);
			}
			time(&now);
		}

		nwrote = sendto(sfd, buf, size, fl,
			(struct sockaddr *) &toAddr,
			(int) sizeof(struct sockaddr_in));

		if (nwrote >= 0)
			break;
		if (errno != EINTR)
			break;		/* Fatal error. */
	}

	return (nwrote);
}	/* SSendto */

#endif



int
SendtoByName(int sfd, const char *const buf, size_t size, const char *const toAddrStr)
{
	int result;
	struct sockaddr_in toAddr;

	if ((result = AddrStrToAddr(toAddrStr, &toAddr, -1)) < 0) {
		return (result);
	}

	do {
		result = sendto(sfd, buf, size, 0,
				(struct sockaddr *) &toAddr,
				(int) sizeof(struct sockaddr_in));
	} while ((result < 0) && (errno == EINTR));

	return (result);
}	/* SendtoByName */
