#include "syshdrs.h"

int
SConnectByName(int sfd, const char * const addrStr, const int tlen)
{
	int result;
	struct sockaddr_in remoteAddr;

	if ((result = AddrStrToAddr(addrStr, &remoteAddr, -1)) == 0) {
		result = SConnect(sfd, &remoteAddr, tlen);
	}
	return (result);
}	/* SConnectByName */
