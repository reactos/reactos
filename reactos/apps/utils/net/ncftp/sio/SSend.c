#include "syshdrs.h"

#ifndef NO_SIGNALS
extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;
#endif

#ifndef NO_SIGNALS

int
SSend(int sfd, char *buf0, size_t size, int fl, int tlen)
{
	volatile int nleft;
	char *volatile buf = buf0;
	int nwrote, tleft;
	vsio_sigproc_t sigalrm, sigpipe;
	time_t done, now;

	if (SSetjmp(gNetTimeoutJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		nwrote = size - nleft;
		if (nwrote > 0)
			return (nwrote);
		errno = ETIMEDOUT;
		return (kTimeoutErr);
	}

	if (SSetjmp(gPipeJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		nwrote = size - nleft;
		if (nwrote > 0)
			return (nwrote);
		errno = EPIPE;
		return (kBrokenPipeErr);
	}

	sigalrm = (vsio_sigproc_t) SSignal(SIGALRM, SIOHandler);
	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIOHandler);

	nleft = (int) size;
	time(&now);
	done = now + tlen;
	forever {
		tleft = (int) (done - now);
		if (tleft < 1) {
			nwrote = size - nleft;
			if (nwrote == 0) {
				nwrote = kTimeoutErr;
				errno = ETIMEDOUT;
			}
			goto done;
		}
		(void) alarm((unsigned int) tleft);
		nwrote = send(sfd, buf, size, fl);
		(void) alarm(0);
		if (nwrote < 0) {
			if (errno != EINTR) {
				nwrote = size - nleft;
				if (nwrote == 0)
					nwrote = -1;
				goto done;
			} else {
				errno = 0;
				nwrote = 0;
				/* Try again. */
			}
		}
		nleft -= nwrote;
		if (nleft <= 0)
			break;
		buf += nwrote;
		time(&now);
	}
	nwrote = size - nleft;

done:
	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);

	return (nwrote);
}	/* SSend */

#else

int
SSend(int sfd, char *buf0, size_t size, int fl, int tlen)
{
	int nleft;
	char *buf = buf0;
	int nwrote, tleft;
	time_t done, now;
	fd_set ss;
	struct timeval tv;
	int result;

	nleft = (int) size;
	time(&now);
	done = now + tlen;
	forever {
		tleft = (int) (done - now);
		if (tleft < 1) {
			nwrote = size - nleft;
			if (nwrote == 0) {
				nwrote = kTimeoutErr;
				errno = ETIMEDOUT;
				SETWSATIMEOUTERR
			}
			goto done;
		}


		/* Unfortunately this doesn't help when the
		 * send buffer fills during the time we're
		 * writing to it, so you could still be
		 * blocked after breaking this loop and starting
		 * the write.
		 */

		forever {
			errno = 0;
			FD_ZERO(&ss);
			FD_SET(sfd, &ss);
			tv.tv_sec = tlen;
			tv.tv_usec = 0;
			result = select(sfd + 1, NULL, SELECT_TYPE_ARG234 &ss, NULL, SELECT_TYPE_ARG5 &tv);
			if (result == 1) {
				/* ready */
				break;
			} else if (result == 0) {
				/* timeout */		
				nwrote = size - nleft;
				if (nwrote > 0)
					return (nwrote);
				errno = ETIMEDOUT;
				SETWSATIMEOUTERR
				return (kTimeoutErr);
			} else if (errno != EINTR) {
				return (-1);
			}
		}

		nwrote = send(sfd, buf, size, fl);

		if (nwrote < 0) {
			if (errno != EINTR) {
				nwrote = size - nleft;
				if (nwrote == 0)
					nwrote = -1;
				goto done;
			} else {
				errno = 0;
				nwrote = 0;
				/* Try again. */
			}
		}
		nleft -= nwrote;
		if (nleft <= 0)
			break;
		buf += nwrote;
		time(&now);
	}
	nwrote = size - nleft;

done:
	return (nwrote);
}	/* SSend */

#endif
