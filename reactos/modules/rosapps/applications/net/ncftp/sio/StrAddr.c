#include "syshdrs.h"

#ifndef INADDR_ANY
#	define INADDR_ANY              ((unsigned long int) 0x00000000)
#endif

#ifndef INADDR_NONE
#	define INADDR_NONE             ((unsigned long int) 0xffffffff)
#endif

/* Linux libc 5.3.x has a bug that causes isalnum() to not work! */
#define ISALNUM(c) ( (((c) >= 'A') && ((c) <= 'Z')) || (((c) >= 'a') && ((c) <= 'z')) || (((c) >= '0') && ((c) <= '9')) )

static unsigned int
ServiceNameToPortNumber(const char *const s)
{
	char str[64];
	char *cp;
	struct servent *sp;

	strncpy(str, s, sizeof(str) - 1);
	str[sizeof(str) - 1] = '\0';
	cp = str;
	if (isdigit(*cp)) {
		while (isdigit(*cp))
			cp++;
		*cp = '\0';
		return (atoi(str));
	}
	for (;; cp++) {
		if ((*cp == '\0')
			|| ((!ISALNUM(*cp)) && (*cp != '-') && (*cp != '_')))
				break;
	}
	*cp = '\0';

	sp = getservbyname(str, "tcp");
	if (sp != NULL) {
		/* endservent(); */
		return ((unsigned int) ntohs((unsigned short) sp->s_port));
	}
	sp = getservbyname(str, "udp");
	if (sp != NULL) {
		/* endservent(); */
		return ((unsigned int) ntohs((unsigned short) sp->s_port));
	}
	return (0);	/* error */
}	/* ServiceNameToPortNumber */




int
AddrStrToAddr(const char * const s, struct sockaddr_in * const sa, const int defaultport)
{
	char portstr[128];
	unsigned long ipnum;
	unsigned int port;
	struct hostent *hp;
	char *hostcp, *atsign, *colon, *cp, *p2;

	memset(sa, 0, sizeof(struct sockaddr_in));
	strncpy(portstr, s, sizeof(portstr));
	portstr[sizeof(portstr) - 1] = '\0';

	if ((colon = strchr(portstr, ':')) != NULL) {
		/* Does it look like a URL?  http://host ? */
		if ((colon[1] == '/') && (colon[2] == '/')) {
			*colon = '\0';
			port = 0;
			hostcp = colon + 3;
			for (cp = hostcp; *cp != '\0'; cp++) {
				if ((!ISALNUM(*cp)) && (*cp != '.')) {
					/* http://host:port */
					if ((*cp == ':') && (isdigit(cp[1]))) {
						*cp++ = '\0';
						p2 = cp;
						while (isdigit(*cp))
							cp++;
						*cp = '\0';
						port = atoi(p2);
					}
					*cp = '\0';
					break;
				}
			}
			if (port == 0)
				port = ServiceNameToPortNumber(portstr);
		} else {
			/* Look for host.name.domain:port */
			*colon = '\0';
			hostcp = portstr;
			port = (unsigned int) atoi(colon + 1);
		}
	} else if ((atsign = strchr(portstr, '@')) != NULL) {
		/* Look for port@host.name.domain */
		*atsign = '\0';
		hostcp = atsign + 1;
		port = (unsigned int) atoi(portstr);
	} else if (defaultport > 0) {
		/* Have just host.name.domain, use that w/ default port. */
		port = (unsigned int) defaultport;
		hostcp = portstr;
	} else {
		/* If defaultport <= 0, they must supply a port number
		 * in the host/port string.
		 */
		errno = EADDRNOTAVAIL;
		return (kAddrStrToAddrMiscErr);
	}

	sa->sin_port = htons((short) port);

	ipnum = inet_addr(hostcp);
	if (ipnum != INADDR_NONE) {
		sa->sin_family = AF_INET;
		sa->sin_addr.s_addr = ipnum;
	} else {
		errno = 0;
		hp = gethostbyname(hostcp);
		if (hp == NULL) {
			if (errno == 0)
				errno = ENOENT;
			return (kAddrStrToAddrBadHost);
		}
		sa->sin_family = hp->h_addrtype;
		memcpy(&sa->sin_addr.s_addr, hp->h_addr_list[0],
			(size_t) hp->h_length);
	}
	return (0);
}	/* AddrStrToAddr */



char *
AddrToAddrStr(char *const dst, size_t dsize, struct sockaddr_in * const saddrp, int dns, const char *fmt)
{
	const char *addrNamePtr;
	struct hostent *hp;
	char str[128];
	char *dlim, *dp;
	const char *cp;
	struct servent *pp;

	if (dns == 0) {
		addrNamePtr = inet_ntoa(saddrp->sin_addr);
	} else {
		hp = gethostbyaddr((char *) &saddrp->sin_addr, (int) sizeof(struct in_addr), AF_INET);
		if ((hp != NULL) && (hp->h_name != NULL) && (hp->h_name[0] != '\0')) {
			addrNamePtr = hp->h_name;
		} else {
			addrNamePtr = inet_ntoa(saddrp->sin_addr);
		}
	}
	if (fmt == NULL)
		fmt = "%h:%p";
	for (dp = dst, dlim = dp + dsize - 1; ; fmt++) {
		if (*fmt == '\0') {
			break;	/* done */
		} else if (*fmt == '%') {
			fmt++;
			if (*fmt == '%') {
				if (dp < dlim)
					*dp++ = '%';
			} else if (*fmt == 'p') {
				sprintf(str, "%u", (unsigned int) ntohs(saddrp->sin_port));
				for (cp = str; *cp != '\0'; cp++)
					if (dp < dlim)
						*dp++ = *cp;
				*dp = '\0';
			} else if (*fmt == 'h') {
				if (addrNamePtr != NULL) {
					cp = addrNamePtr;
				} else {
					cp = "unknown";
				}
				for ( ; *cp != '\0'; cp++)
					if (dp < dlim)
						*dp++ = *cp;
				*dp = '\0';
			} else if (*fmt == 's') {
				pp = getservbyport((int) (saddrp->sin_port), "tcp");
				if (pp == NULL)
					pp = getservbyport((int) (saddrp->sin_port), "udp");
				if (pp == NULL) {
					sprintf(str, "%u", (unsigned int) ntohs(saddrp->sin_port));
					cp = str;
				} else {
					cp = pp->s_name;
				}
				for ( ; *cp != '\0'; cp++)
					if (dp < dlim)
						*dp++ = *cp;
				/* endservent(); */
				*dp = '\0';
			} else if ((*fmt == 't') || (*fmt == 'u')) {
				pp = getservbyport((int) (saddrp->sin_port), (*fmt == 'u') ? "udp" : "tcp");
				if (pp == NULL) {
					sprintf(str, "%u", (unsigned int) ntohs(saddrp->sin_port));
					cp = str;
				} else {
					cp = pp->s_name;
				}
				for ( ; *cp != '\0'; cp++)
					if (dp < dlim)
						*dp++ = *cp;
				/* endservent(); */
				*dp = '\0';
			} else if (*fmt == '\0') {
				break;
			} else {
				if (dp < dlim)
					*dp++ = *fmt;
			}
		} else if (dp < dlim) {
			*dp++ = *fmt;
		}
	}
	*dp = '\0';
	return (dst);
}	/* AddrToAddrStr */
