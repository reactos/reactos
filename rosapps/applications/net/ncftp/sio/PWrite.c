#include "syshdrs.h"

#if !defined(NO_SIGNALS) && defined(SIGPIPE)
extern volatile Sjmp_buf gPipeJmp;
#endif
int
PWrite(int sfd, const char *const buf0, size_t size)
{
	volatile int nleft;
	const char *volatile buf = buf0;
	int nwrote;
#if !defined(NO_SIGNALS) && defined(SIGPIPE)
	vsio_sigproc_t sigpipe;

	if (SSetjmp(gPipeJmp) != 0) {
		(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
		nwrote = size - nleft;
		if (nwrote > 0)
			return (nwrote);
		errno = EPIPE;
		return (kBrokenPipeErr);
	}

	sigpipe = (vsio_sigproc_t) SSignal(SIGPIPE, SIOHandler);
#endif

	nleft = (int) size;
	forever {
		nwrote = write(sfd, buf, nleft);
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
	}
	nwrote = size - nleft;

done:
#if !defined(NO_SIGNALS) && defined(SIGPIPE)
	(void) SSignal(SIGPIPE, (sio_sigproc_t) sigpipe);
#endif

	return (nwrote);
}	/* PWrite */
