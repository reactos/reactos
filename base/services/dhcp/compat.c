#include "rosdhcp.h"
#include "dhcpd.h"
#include "stdint.h"

size_t strlcpy(char *d, const char *s, size_t bufsize)
{
        size_t len = strlen(s);
        size_t ret = len;
        if (bufsize > 0) {
                if (len >= bufsize)
                        len = bufsize-1;
                memcpy(d, s, len);
                d[len] = 0;
        }
        return ret;
}

// not really random :(
u_int32_t arc4random()
{
	static int did_srand = 0;
	u_int32_t ret;

	if (!did_srand) {
		srand(0);
		did_srand = 1;
	}

	ret = rand() << 10 ^ rand();
	return ret;
}


int inet_aton(const char *cp, struct in_addr *inp)
{
	inp->S_un.S_addr = inet_addr(cp);
	if (INADDR_NONE == inp->S_un.S_addr)
		return 0;

	return 1;
}

