#include "syshdrs.h"

int
SAcceptS(int sfd, struct sockaddr_in *const addr, int tlen)
{
	int result;
	fd_set ss;
	struct timeval tv;
	size_t size;

	if (tlen <= 0) {
		errno = 0;
		for (;;) {
			size = sizeof(struct sockaddr_in);
			result = accept(sfd, (struct sockaddr *) addr, (int *) &size);
			if ((result >= 0) || (errno != EINTR))
				return (result);
		}
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
			errno = ETIMEDOUT;
			SETWSATIMEOUTERR
			return (kTimeoutErr);
		} else if (errno != EINTR) {
			return (-1);
		}
	}

	do {
		size = sizeof(struct sockaddr_in);
		result = accept(sfd, (struct sockaddr *) addr, (int *) &size);
	} while ((result < 0) && (errno == EINTR));

	return (result);
}	/* SAcceptS */
