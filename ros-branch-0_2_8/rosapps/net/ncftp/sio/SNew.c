#include "syshdrs.h"

int
SNewStreamClient(void)
{
	int sfd;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		return kSNewFailed;

	return (sfd);
}	/* SNewStreamClient */




int
SNewDatagramClient(void)
{
	int sfd;

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0)
		return kSNewFailed;

	return (sfd);
}	/* SNewDatagramClient */




int
SNewStreamServer(const int port, const int nTries, const int reuseFlag, int listenQueueSize)
{
	int oerrno;
	int sfd;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		return kSNewFailed;

	if (SBind(sfd, port, nTries, reuseFlag) < 0) {
		oerrno = errno;
		(void) closesocket(sfd);
		errno = oerrno;
		return kSBindFailed;
	}

	if (SListen(sfd, listenQueueSize) < 0) {
		oerrno = errno;
		(void) closesocket(sfd);
		errno = oerrno;
		return kSListenFailed;
	}

	return (sfd);
}	/* SNewStreamServer */




int
SNewDatagramServer(const int port, const int nTries, const int reuseFlag)
{
	int oerrno;
	int sfd;

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0)
		return kSNewFailed;

	if (SBind(sfd, port, nTries, reuseFlag) < 0) {
		oerrno = errno;
		(void) closesocket(sfd);
		errno = oerrno;
		return kSBindFailed;
	}

	return (sfd);
}	/* SNewDatagramServer */
