#include "syshdrs.h"

#if !defined(NO_UNIX_DOMAIN_SOCKETS)

int
UAcceptS(int sfd, struct sockaddr_un *const addr, int *ualen, int tlen)
{
	int result;
	fd_set ss;
	struct timeval tv;

	if (tlen < 0) {
		errno = 0;
		for (;;) {
			*ualen = (int) sizeof(struct sockaddr_un);
			result = accept(sfd, (struct sockaddr *) addr, (int *) ualen);
			if ((result >= 0) || (errno != EINTR))
				return (result);
		}
	}

	for (;;) {
		errno = 0;
		FD_ZERO(&ss);
		FD_SET(sfd, &ss);
		tv.tv_sec = tlen;
		tv.tv_usec = 0;
		result = select(sfd + 1, SELECT_TYPE_ARG234 &ss, NULL, NULL, &tv);
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
	}

	do {
		*ualen = (int) sizeof(struct sockaddr_un);
		result = accept(sfd, (struct sockaddr *) addr, (int *) ualen);
	} while ((result < 0) && (errno == EINTR));

	return (result);
}	/* UAcceptS */

#endif

