#include "syshdrs.h"

#ifndef NO_SIGNALS
extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;
#endif

#ifndef NO_SIGNALS

int
SRecv(int sfd, char *const buf0, size_t size, int fl, int tlen, int retry)
{
	int nread;
	volatile int nleft;
	char *volatile buf = buf0;
	int tleft;
	vsio_sigproc_t sigalrm, sigpipe;
	time_t done, now;

	if (SSetjmp(gNetTimeoutJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		nread = size - nleft;
		if ((nread > 0) && (retry == kFullBufferNotRequired))
			return (nread);
		errno = ETIMEDOUT;
		return (kTimeoutErr);
	}

	if (SSetjmp(gPipeJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		nread = size - nleft;
		if ((nread > 0) && (retry == kFullBufferNotRequired))
			return (nread);
		errno = EPIPE;
		return (kBrokenPipeErr);
	}

	sigalrm = (vsio_sigproc_t) SSignal(SIGALRM, SIOHandler);
	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIOHandler);
	errno = 0;

	nleft = (int) size;
	time(&now);
	done = now + tlen;
	forever {
		tleft = (int) (done - now);
		if (tleft < 1) {
			nread = size - nleft;
			if ((nread == 0) || (retry == kFullBufferRequired)) {
				nread = kTimeoutErr;
				errno = ETIMEDOUT;
			}
			goto done;
		}
		(void) alarm((unsigned int) tleft);
		nread = recv(sfd, (char *) buf, nleft, fl);
		(void) alarm(0);
		if (nread <= 0) {
			if (nread == 0) {
				/* EOF */
				if (retry == kFullBufferRequiredExceptLast)
					nread = size - nleft;
				goto done;
			} else if (errno != EINTR) {
				nread = size - nleft;
				if (nread == 0)
					nread = -1;
				goto done;
			} else {
				errno = 0;
				nread = 0;
				/* Try again. */
			}
		}
		nleft -= nread;
		if ((nleft <= 0) || ((retry == 0) && (nleft != (int) size)))
			break;
		buf += nread;
		time(&now);
	}
	nread = size - nleft;

done:
	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);

	return (nread);
}	/* SRecv */

#else

int
SRecv(int sfd, char *const buf0, size_t size, int fl, int tlen, int retry)
{
	int nread;
	int nleft;
	char *buf = buf0;
	int tleft;
	time_t done, now;
	fd_set ss;
	struct timeval tv;
	int result;

	errno = 0;

	nleft = (int) size;
	time(&now);
	done = now + tlen;
	forever {
		tleft = (int) (done - now);
		if (tleft < 1) {
			nread = size - nleft;
			if ((nread == 0) || (retry == kFullBufferRequired)) {
				nread = kTimeoutErr;
				errno = ETIMEDOUT;
				SETWSATIMEOUTERR
			}
			goto done;
		}

		forever {
			errno = 0;
			FD_ZERO(&ss);
			FD_SET(sfd, &ss);
			tv.tv_sec = tlen;
			tv.tv_usec = 0;
			result = select(sfd + 1, SELECT_TYPE_ARG234 &ss, NULL, NULL, SELECT_TYPE_ARG5 &tv);
			if (result == 1) {
				/* ready */
				break;
			} else if (result == 0) {
				/* timeout */		
				nread = size - nleft;
				if ((nread > 0) && (retry == kFullBufferNotRequired))
					return (nread);
				errno = ETIMEDOUT;
				SETWSATIMEOUTERR
				return (kTimeoutErr);
			} else if (errno != EINTR) {
				return (-1);
			}
		}

#if defined(WIN32) || defined(_WINDOWS)
		nread = recv(sfd, (char *) buf, nleft, fl);
#else
		nread = recv(sfd, (char *) buf, nleft, fl);
#endif

		if (nread <= 0) {
			if (nread == 0) {
				/* EOF */
				if (retry == kFullBufferRequiredExceptLast)
					nread = size - nleft;
				goto done;
			} else if (errno != EINTR) {
				nread = size - nleft;
				if (nread == 0)
					nread = -1;
				goto done;
			} else {
				errno = 0;
				nread = 0;
				/* Try again. */
			}
		}
		nleft -= nread;
		if ((nleft <= 0) || ((retry == 0) && (nleft != (int) size)))
			break;
		buf += nread;
		time(&now);
	}
	nread = size - nleft;

done:
	return (nread);
}	/* SRecv */

#endif

