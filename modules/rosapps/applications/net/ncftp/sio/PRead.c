#include "syshdrs.h"

#if !defined(NO_SIGNALS) && defined(SIGPIPE)
extern volatile Sjmp_buf gPipeJmp;
#endif

/* Read up to "size" bytes on sfd.
 *
 * If "retry" is on, after a successful read of less than "size"
 * bytes, it will attempt to read more, upto "size."
 *
 * Although "retry" would seem to indicate you may want to always
 * read "size" bytes or else it is an error, even with that on you
 * may get back a value < size.  Set "retry" to 0 when you want to
 * return as soon as there is a chunk of data whose size is <= "size".
 */

int
PRead(int sfd, char *const buf0, size_t size, int retry)
{
	int nread;
	volatile int nleft;
	char *volatile buf = buf0;
#if !defined(NO_SIGNALS) && defined(SIGPIPE)
	vsio_sigproc_t sigpipe;

	if (SSetjmp(gPipeJmp) != 0) {
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		nread = size - nleft;
		if (nread > 0)
			return (nread);
		errno = EPIPE;
		return (kBrokenPipeErr);
	}

	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIOHandler);
#endif
	errno = 0;

	nleft = (int) size;
	forever {
		nread = read(sfd, buf, nleft);
		if (nread <= 0) {
			if (nread == 0) {
				/* EOF */
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
		if ((nleft <= 0) || (retry == 0))
			break;
		buf += nread;
	}
	nread = size - nleft;

done:
#if !defined(NO_SIGNALS) && defined(SIGPIPE)
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
#endif
	return (nread);
}	/* PRead */
