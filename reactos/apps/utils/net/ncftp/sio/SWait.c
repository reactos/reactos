#include "syshdrs.h"

/* 
 * Return zero if the operation timed-out or erred-out, otherwise non-zero.
 */
int
SWaitUntilReadyForReading(const int sfd, const int tlen)
{
	fd_set ss, ss2;
	struct timeval tv;
	int result;
	int tleft;
	time_t now, done;

	if (sfd < 0) {
		errno = EBADF;
		return (0);
	}

	time(&now);
	done = now + tlen;
	tleft = tlen;

	forever {
		FD_ZERO(&ss);
		FD_SET(sfd, &ss);
		ss2 = ss;
		tv.tv_sec = tleft;
		tv.tv_usec = 0;
		result = select(sfd + 1, SELECT_TYPE_ARG234 &ss, NULL, SELECT_TYPE_ARG234 &ss2, &tv);
		if (result == 1) {
			/* ready */
			return (1);
		} else if (result < 0) {
			if (errno != EINTR) {
				/* error */
				return (0);
			}
			/* try again */
			time(&now);
			if (now > done) {
				/* timed-out */
				errno = ETIMEDOUT;
				return (0);
			}
			tleft = (int) (done - now);
		} else {
			/* timed-out */
			errno = ETIMEDOUT;
			return (0);
		}
	}
}	/* SWaitUntilReadyForReading */




/* 
 * Return zero if the operation timed-out or erred-out, otherwise non-zero.
 */
int
SWaitUntilReadyForWriting(const int sfd, const int tlen)
{
	fd_set ss, ss2;
	struct timeval tv;
	int result;
	int tleft;
	time_t now, done;

	if (sfd < 0) {
		errno = EBADF;
		return (0);
	}

	time(&now);
	done = now + tlen;
	tleft = tlen;

	forever {
		FD_ZERO(&ss);
		FD_SET(sfd, &ss);
		ss2 = ss;
		tv.tv_sec = tleft;
		tv.tv_usec = 0;
		result = select(sfd + 1, NULL, SELECT_TYPE_ARG234 &ss, SELECT_TYPE_ARG234 &ss2, &tv);
		if (result == 1) {
			/* ready */
			return (1);
		} else if (result < 0) {
			if (errno != EINTR) {
				/* error */
				return (0);
			}
			/* try again */
			time(&now);
			if (now > done) {
				/* timed-out */
				errno = ETIMEDOUT;
				return (0);
			}
			tleft = (int) (done - now);
		} else {
			/* timed-out */
			errno = ETIMEDOUT;
			return (0);
		}
	}
}	/* SWaitUntilReadyForWriting */
