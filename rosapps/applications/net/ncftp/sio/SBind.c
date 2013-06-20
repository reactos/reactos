#include "syshdrs.h"

int
SBind(int sockfd, const int port, const int nTries, const int reuseFlag)
{
	unsigned int i;
	int on;
	int onsize;
	struct sockaddr_in localAddr;

	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons((unsigned short) port);

	if (reuseFlag != kReUseAddrNo) {
		/* This is mostly so you can quit the server and re-run it
		 * again right away.  If you don't do this, the OS may complain
		 * that the address is still in use.
		 */
		on = 1;
		onsize = (int) sizeof(on);
		(void) setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
			(char *) &on, onsize);

#ifdef SO_REUSEPORT
		/* Tells kernel that it's okay to have more
		 * than one process originating from this
		 * local port.
		 */
		on = 1;
		onsize = (int) sizeof(on);
		(void) setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
			(char *) &on, onsize);
#endif	/* SO_REUSEPORT */
	}

	for (i=1; ; i++) {
		/* Try binding a few times, in case we get Address in Use
		 * errors.
		 */
		if (bind(sockfd, (struct sockaddr *) &localAddr, sizeof(struct sockaddr_in)) == 0) {
			break;
		}
		if ((int) i == nTries) {
			return (-1);
		}
		/* Give the OS time to clean up the old socket,
		 * and then try again.
		 */
		sleep(i * 3);
	}

	return (0);
}	/* SBind */




int
SListen(int sfd, int backlog)
{
	return (listen(sfd, (unsigned int) backlog));
}	/* SListen */
