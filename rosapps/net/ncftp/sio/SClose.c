#include "syshdrs.h"

#ifndef NO_SIGNALS
extern volatile Sjmp_buf gNetTimeoutJmp;
extern volatile Sjmp_buf gPipeJmp;
#endif

int
SClose(int sfd, int tlen)
{
#ifndef NO_SIGNALS
	vsio_sigproc_t sigalrm, sigpipe;

	if (sfd < 0) {
		errno = EBADF;
		return (-1);
	}

	if (tlen < 1) {
		/* Don't time it, shut it down now. */
		if (SetSocketLinger(sfd, 0, 0) == 0) {
			/* Linger disabled, so close()
			 * should not block.
			 */
			return (closesocket(sfd));
		} else {
			/* This will result in a fd leak,
			 * but it's either that or hang forever.
			 */
			return (shutdown(sfd, 2));
		}
	}

	if (SSetjmp(gNetTimeoutJmp) != 0) {
		alarm(0);
		(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		if (SetSocketLinger(sfd, 0, 0) == 0) {
			/* Linger disabled, so close()
			 * should not block.
			 */
			return closesocket(sfd);
		} else {
			/* This will result in a fd leak,
			 * but it's either that or hang forever.
			 */
			(void) shutdown(sfd, 2);
		}
		return (-1);
	}

	sigalrm = (vsio_sigproc_t) SSignal(SIGALRM, SIOHandler);
	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIG_IGN);

	alarm((unsigned int) tlen);
	for (;;) {
		if (closesocket(sfd) == 0) {
			errno = 0;
			break;
		}
		if (errno != EINTR)
			break;
	} 
	alarm(0);
	(void) SSignal(SIGALRM, (sio_sigproc_t) sigalrm);

	if ((errno != 0) && (errno != EBADF)) {
		if (SetSocketLinger(sfd, 0, 0) == 0) {
			/* Linger disabled, so close()
			 * should not block.
			 */
			(void) closesocket(sfd);
		} else {
			/* This will result in a fd leak,
			 * but it's either that or hang forever.
			 */
			(void) shutdown(sfd, 2);
		}
	}
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);

	return ((errno == 0) ? 0 : (-1));
#else
	struct timeval tv;
	int result;
	time_t done, now;
	fd_set ss;

	if (sfd < 0) {
		errno = EBADF;
		return (-1);
	}

	if (tlen < 1) {
		/* Don't time it, shut it down now. */
		if (SetSocketLinger(sfd, 0, 0) == 0) {
			/* Linger disabled, so close()
			 * should not block.
			 */
			return (closesocket(sfd));
		} else {
			/* This will result in a fd leak,
			 * but it's either that or hang forever.
			 */
			return (shutdown(sfd, 2));
		}
	}

	/* Wait until the socket is ready for writing (usually easy). */
	time(&now);
	done = now + tlen;

	forever {
		tlen = done - now;
		if (tlen <= 0) {
			/* timeout */
			if (SetSocketLinger(sfd, 0, 0) == 0) {
				/* Linger disabled, so close()
				 * should not block.
				 */
				(void) closesocket(sfd);
			} else {
				/* This will result in a fd leak,
				 * but it's either that or hang forever.
				 */
				(void) shutdown(sfd, 2);
			}
			errno = ETIMEDOUT;
			return (kTimeoutErr);
		}

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
			if (SetSocketLinger(sfd, 0, 0) == 0) {
				/* Linger disabled, so close()
				 * should not block.
				 */
				(void) closesocket(sfd);
			} else {
				/* This will result in a fd leak,
				 * but it's either that or hang forever.
				 */
				(void) shutdown(sfd, 2);
			}
			errno = ETIMEDOUT;
			return (kTimeoutErr);
		} else if (errno != EINTR) {
			/* Error, done. This end may have been shutdown. */
			break;
		}
		time(&now);
	}

	/* Wait until the socket is ready for reading. */
	forever {
		tlen = done - now;
		if (tlen <= 0) {
			/* timeout */
			if (SetSocketLinger(sfd, 0, 0) == 0) {
				/* Linger disabled, so close()
				 * should not block.
				 */
				(void) closesocket(sfd);
			} else {
				/* This will result in a fd leak,
				 * but it's either that or hang forever.
				 */
				(void) shutdown(sfd, 2);
			}
			errno = ETIMEDOUT;
			return (kTimeoutErr);
		}

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
			if (SetSocketLinger(sfd, 0, 0) == 0) {
				/* Linger disabled, so close()
				 * should not block.
				 */
				(void) closesocket(sfd);
			} else {
				/* This will result in a fd leak,
				 * but it's either that or hang forever.
				 */
				(void) shutdown(sfd, 2);
			}
			errno = ETIMEDOUT;
			return (kTimeoutErr);
		} else if (errno != EINTR) {
			/* Error, done. This end may have been shutdown. */
			break;
		}
		time(&now);
	}

	/* If we get here, close() won't block. */
	return closesocket(sfd);
#endif
}	/* SClose */
