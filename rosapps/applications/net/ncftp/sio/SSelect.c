#include "syshdrs.h"

void
SelectSetInit(SelectSetPtr const ssp, const double timeout)
{
	double i;
	long l;

	/* Inititalize SelectSet, which will clear the fd_set, the
	 * timeval, and the maxfd and numfds to 0.
	 */
	memset(ssp, 0, sizeof(SelectSet));
	l = (long) timeout;
	i = (double) l;
	ssp->timeout.tv_sec = l;
	ssp->timeout.tv_usec = (long) ((timeout - i) * 1000000.0);
}	/* SelectSetInit */




void
SelectSetAdd(SelectSetPtr const ssp, const int fd)
{
	if (fd >= 0) {
		FD_SET(fd, &ssp->fds);
		if (ssp->maxfd < (fd + 1))
			ssp->maxfd = (fd + 1);
		++ssp->numfds;
	}
}	/* SelectSetAdd */




void
SelectSetRemove(SelectSetPtr const ssp, const int fd)
{
	if ((fd >= 0) && (FD_ISSET(fd, &ssp->fds))) {
		FD_CLR(fd, &ssp->fds);
		/* Note that maxfd is left alone, even if maxfd was
		 * this one.  That is okay.
		 */
		--ssp->numfds;
	}
}	/* SelectSetRemove */



int
SelectW(SelectSetPtr ssp, SelectSetPtr resultssp)
{
	int rc;

	do {
		memcpy(resultssp, ssp, sizeof(SelectSet));
		rc = select(resultssp->maxfd, NULL, SELECT_TYPE_ARG234 &resultssp->fds, NULL, SELECT_TYPE_ARG5 &resultssp->timeout);
	} while ((rc < 0) && (errno == EINTR));
	return (rc);
}	/* SelectW */



int
SelectR(SelectSetPtr ssp, SelectSetPtr resultssp)
{
	int rc;

	do {
		memcpy(resultssp, ssp, sizeof(SelectSet));
		rc = select(resultssp->maxfd, SELECT_TYPE_ARG234 &resultssp->fds, NULL, NULL, SELECT_TYPE_ARG5 &resultssp->timeout);
	} while ((rc < 0) && (errno == EINTR));
	return (rc);
}	/* SelectR */
