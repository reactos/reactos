/* ftp.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#define _libncftp_ftp_c_
#include "syshdrs.h"

char gLibNcFTPVersion[64] = kLibraryVersion;

#ifdef NO_SIGNALS
static char gNoSignalsMarker[] = "@(#) LibNcFTP - NO_SIGNALS";
#else

static int gGotSig = 0;
#ifdef HAVE_SIGSETJMP
static sigjmp_buf gCancelConnectJmp;
#else
static jmp_buf gCancelConnectJmp;
#endif	/* HAVE_SIGSETJMP */

#endif	/* NO_SIGNALS */


#ifndef lint
static char gCopyright[] = "@(#) LibNcFTP Copyright 1995-2000, by Mike Gleason.  All rights reserved.";
#endif

#ifdef HAVE_LIBSOCKS5
#	define SOCKS 5
#	include <socks.h>
#else
#	ifdef HAVE_LIBSOCKS
#		define accept		Raccept
#		define connect		Rconnect
#		define getsockname	Rgetsockname
#		define listen		Rlisten
#	endif
#endif




/* On entry, you should have 'host' be set to a symbolic name (like
 * cse.unl.edu), or set to a numeric address (like 129.93.3.1).
 * If the function fails, it will return NULL, but if the host was
 * a numeric style address, you'll have the ip_address to fall back on.
 */

static struct hostent *
GetHostEntry(char *host, struct in_addr *ip_address)
{
	struct in_addr ip;
	struct hostent *hp;
	
	/* See if the host was given in the dotted IP format, like "36.44.0.2."
	 * If it was, inet_addr will convert that to a 32-bit binary value;
	 * it not, inet_addr will return (-1L).
	 */
	ip.s_addr = inet_addr(host);
	if (ip.s_addr != INADDR_NONE) {
		hp = NULL;
	} else {
		/* No IP address, so it must be a hostname, like ftp.wustl.edu. */
		hp = gethostbyname(host);
		if (hp != NULL)
			(void) memcpy(&ip.s_addr, hp->h_addr_list[0], (size_t) hp->h_length);
	}
	if (ip_address != NULL)
		*ip_address = ip;
	return (hp);
}	/* GetHostEntry */




/* Makes every effort to return a fully qualified domain name. */
int
GetOurHostName(char *host, size_t siz)
{
#ifdef HOSTNAME
	/* You can hardcode in the name if this routine doesn't work
	 * the way you want it to.
	 */
	Strncpy(host, HOSTNAME, siz);
	return (1);		/* Success */
#else
	struct hostent *hp;
	int result;
	char **curAlias;
	char domain[64];
	char *cp;
	int rc;

	host[0] = '\0';
	result = gethostname(host, (int) siz);
	if ((result < 0) || (host[0] == '\0')) {
		return (-1);
	}

	if (strchr(host, '.') != NULL) {
		/* gethostname returned full name (like "cse.unl.edu"), instead
		 * of just the node name (like "cse").
		 */
		return (2);		/* Success */
	}
	
	hp = gethostbyname(host);
	if (hp != NULL) {
		/* Maybe the host entry has the full name. */
		cp = strchr((char *) hp->h_name, '.');
		if ((cp != NULL) && (cp[1] != '\0')) {
			/* The 'name' field for the host entry had full name. */
			(void) Strncpy(host, (char *) hp->h_name, siz);
			return (3);		/* Success */
		}

		/* Now try the list of aliases, to see if any of those look real. */
		for (curAlias = hp->h_aliases; *curAlias != NULL; curAlias++) {
			cp = strchr(*curAlias, '.');
			if ((cp != NULL) && (cp[1] != '\0')) {
				(void) Strncpy(host, *curAlias, siz);
				return (4);		/* Success */
			}
		}
	}

	/* Otherwise, we just have the node name.  See if we can get the
	 * domain name ourselves.
	 */
#ifdef DOMAINNAME
	(void) STRNCPY(domain, DOMAINNAME);
	rc = 5;
#else
	rc = -1;
	domain[0] = '\0';
#	if defined(HAVE_RES_INIT) && defined(HAVE__RES_DEFDNAME)
	if (domain[0] == '\0') {
		(void) res_init();
		if ((_res.defdname != NULL) && (_res.defdname[0] != '\0')) {
			(void) STRNCPY(domain, _res.defdname);
			rc = 6;
		}
	}
#	endif	/* HAVE_RES_INIT && HAVE__RES_DEFDNAME */
	
	if (domain[0] == '\0') {
		FILE *fp;
		char line[256];
		char *tok;

		fp = fopen("/etc/resolv.conf", "r");
		if (fp != NULL) {
			(void) memset(line, 0, sizeof(line));
			while (fgets(line, sizeof(line) - 1, fp) != NULL) {
				if (!isalpha((int) line[0]))
					continue;	/* Skip comment lines. */
				tok = strtok(line, " \t\n\r");
				if (tok == NULL)
					continue;	/* Impossible */
				if (strcmp(tok, "domain") == 0) {
					tok = strtok(NULL, " \t\n\r");
					if (tok == NULL)
						continue;	/* syntax error */
					(void) STRNCPY(domain, tok);
					rc = 7;
					break;	/* Done. */
				}
			}
			(void) fclose(fp);
		}
	}
#endif	/* DOMAINNAME */

	if (domain[0] != '\0') {
		/* Supposedly, it's legal for a domain name with
		 * a period at the end.
		 */
		cp = domain + strlen(domain) - 1;
		if (*cp == '.')
			*cp = '\0';
		if (domain[0] != '.')
			(void) Strncat(host, ".", siz);
		(void) Strncat(host, domain, siz);
	}
	if (rc < 0)
		host[0] = '\0';
	return(rc);	/* Success */
#endif	/* !HOSTNAME */
}	/* GetOurHostName */



void
CloseControlConnection(const FTPCIPtr cip)
{
	/* This will close each file, if it was open. */
#ifdef NO_SIGNALS
	SClose(cip->ctrlSocketR, 3);
	cip->ctrlSocketR = kClosedFileDescriptor;
	cip->ctrlSocketW = kClosedFileDescriptor;
	DisposeSReadlineInfo(&cip->ctrlSrl);
#else	/* NO_SIGNALS */
	if (cip->ctrlTimeout > 0)
		(void) alarm(cip->ctrlTimeout);
	CloseFile(&cip->cin);
	CloseFile(&cip->cout);
	cip->ctrlSocketR = kClosedFileDescriptor;
	cip->ctrlSocketW = kClosedFileDescriptor;
	if (cip->ctrlTimeout > 0)
		(void) alarm(0);
#endif	/* NO_SIGNALS */
	cip->connected = 0;
	cip->loggedIn = 0;
}	/* CloseControlConnection */



static int
GetSocketAddress(const FTPCIPtr cip, int sockfd, struct sockaddr_in *saddr)
{
	int len = (int) sizeof (struct sockaddr_in);
	int result = 0;

	if (getsockname(sockfd, (struct sockaddr *)saddr, &len) < 0) {
		Error(cip, kDoPerror, "Could not get socket name.\n");
		cip->errNo = kErrGetSockName;
		result = kErrGetSockName;
	}
	return (result);
}	/* GetSocketAddress */




int
SetKeepAlive(const FTPCIPtr cip, int sockfd)
{
#ifndef SO_KEEPALIVE
	cip->errNo = kErrSetKeepAlive;
	return (kErrSetKeepAlive);
#else
	int opt;

	opt = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, (int) sizeof(opt)) < 0) {
		/* Error(cip, kDoPerror, "Could not set keep-alive mode.\n"); */
		cip->errNo = kErrSetKeepAlive;
		return (kErrSetKeepAlive);
	}
	return (kNoErr);
#endif	/* SO_KEEPALIVE */
}	/* SetKeepAlive */




int
SetLinger(const FTPCIPtr cip, int sockfd, int onoff)
{
#ifndef SO_LINGER
	cip->errNo = kErrSetLinger;
	return (kErrSetLinger);
#else
	struct linger li;

	if (onoff != 0) {
		li.l_onoff = 1;
		li.l_linger = 120;	/* 2 minutes, but system ignores field. */
	} else {
		li.l_onoff = 0;
		li.l_linger = 0;
	}
	/* Have the system make an effort to deliver any unsent data,
	 * even after we close the connection.
	 */
	if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char *) &li, (int) sizeof(li)) < 0) {
		/* Error(cip, kDoPerror, "Could not set linger mode.\n"); */
		cip->errNo = kErrSetLinger;
		return (kErrSetLinger);
	}
	return (kNoErr);
#endif	/* SO_LINGER */
}	/* SetLinger */




#ifdef IP_TOS
int
SetTypeOfService(const FTPCIPtr cip, int sockfd, int tosType)
{
	/* Specify to the router what type of connection this is, so it
	 * can prioritize packets.
	 */
	if (setsockopt(sockfd, IPPROTO_IP, IP_TOS, (char *) &tosType, (int) sizeof(tosType)) < 0) {
		/* Error(cip, kDoPerror, "Could not set type of service.\n"); */
		cip->errNo = kErrSetTypeOfService;
		return (kErrSetTypeOfService);
	}
	return (kNoErr);
}	/* SetTypeOfService */
#endif	/* IP_TOS */




#ifdef SO_OOBINLINE
int
SetInlineOutOfBandData(const FTPCIPtr cip, int sockfd)
{
	int on = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_OOBINLINE, (char *) &on, (int) sizeof(on)) < 0) {
		Error(cip, kDoPerror, "Could not set out of band inline mode.\n");
		cip->errNo = kErrSetOutOfBandInline;
		return (kErrSetOutOfBandInline);
	}
	return (kNoErr);
}	/* SetInlineOutOfBandData */
#endif /* SO_OOBINLINE */




#ifndef NO_SIGNALS

static void
CancelConnect(int signum)
{
	gGotSig = signum;
#ifdef HAVE_SIGSETJMP
	siglongjmp(gCancelConnectJmp, 1);
#else
	longjmp(gCancelConnectJmp, 1);
#endif	/* HAVE_SIGSETJMP */
}	/* CancelConnect */

#endif	/* NO_SIGNALS */



int
OpenControlConnection(const FTPCIPtr cip, char *host, unsigned int port)
{
	struct in_addr ip_address;
	int err = 0;
	int result;
	int oerrno;
	volatile int sockfd = -1;
	volatile int sock2fd = -1;
	ResponsePtr rp;
	char **volatile curaddr;
	struct hostent *hp;
	char *volatile fhost;
	unsigned int fport;
#ifndef NO_SIGNALS
	volatile FTPSigProc osigint;
	volatile FTPSigProc osigalrm;
	volatile FTPCIPtr vcip;
	int sj;
#endif	/* NO_SIGNALS */
	const char *firstLine, *secondLine, *srvr;

	LIBNCFTP_USE_VAR(gLibNcFTPVersion);
	LIBNCFTP_USE_VAR(gCopyright);
#ifdef NO_SIGNALS
	LIBNCFTP_USE_VAR(gNoSignalsMarker);
#endif	/* NO_SIGNALS */

	if (cip->firewallType == kFirewallNotInUse) {
		fhost = host;
		fport = port;
	} else {
		fhost = cip->firewallHost;
		fport = cip->firewallPort;
	}
	if (fport == 0)
		fport = cip->lip->defaultPort;

	/* Since we're the client, we just have to get a socket() and
	 * connect() it.
	 */
	(void) ZERO(cip->servCtlAddr);
	cip->cin = NULL;
	cip->cout = NULL;

	/* Make sure we use network byte-order. */
	fport = (unsigned int) htons((unsigned short) fport);

	cip->servCtlAddr.sin_port = (unsigned short) fport;

	hp = GetHostEntry(fhost, &ip_address);

	if (hp == NULL) {
		/* Okay, no Host entry, but maybe we have a numeric address
		 * in ip_address we can try.
		 */
		if (ip_address.s_addr == INADDR_NONE) {
			Error(cip, kDontPerror, "%s: unknown host.\n", fhost);
			cip->errNo = kErrHostUnknown;
			return (kErrHostUnknown);
		}
		cip->servCtlAddr.sin_family = AF_INET;
		cip->servCtlAddr.sin_addr.s_addr = ip_address.s_addr;
	} else {
		cip->servCtlAddr.sin_family = hp->h_addrtype;
		/* We'll fill in the rest of the structure below. */
	}
	
	/* After obtaining a socket, try to connect it to a remote
	 * address.  If we didn't get a host entry, we will only have
	 * one thing to try (ip_address);  if we do have one, we can try
	 * every address in the list from the host entry.
	 */

	if (hp == NULL) {
		/* Since we're given a single raw address, and not a host entry,
		 * we can only try this one address and not any other addresses
		 * that could be present for a site with a host entry.
		 */

		if ((sockfd = socket(cip->servCtlAddr.sin_family, SOCK_STREAM, 0)) < 0) {
			Error(cip, kDoPerror, "Could not get a socket.\n");
			cip->errNo = kErrNewStreamSocket;
			return (kErrNewStreamSocket);
		}

		/* This doesn't do anything if you left these
		 * at their defaults (zero).  Otherwise it
		 * tries to set the buffer size to the
		 * size specified.
		 */
		(void) SetSockBufSize(sockfd, cip->ctrlSocketRBufSize, cip->ctrlSocketSBufSize);

#ifdef NO_SIGNALS
		err = SConnect(sockfd, &cip->servCtlAddr, (int) cip->connTimeout);

		if (err < 0) {
			oerrno = errno;
			(void) SClose(sockfd, 3);
			errno = oerrno;
			sockfd = -1;
		}
#else	/* NO_SIGNALS */
		osigint = (volatile FTPSigProc) signal(SIGINT, CancelConnect);
		if (cip->connTimeout > 0) {
			osigalrm = (volatile FTPSigProc) signal(SIGALRM, CancelConnect);
			(void) alarm(cip->connTimeout);
		}

		vcip = cip;

#ifdef HAVE_SIGSETJMP
		sj = sigsetjmp(gCancelConnectJmp, 1);
#else
		sj = setjmp(gCancelConnectJmp);
#endif	/* HAVE_SIGSETJMP */

		if (sj != 0) {
			/* Interrupted by a signal. */
			(void) closesocket(sockfd);
			(void) signal(SIGINT, (FTPSigProc) osigint);
			if (vcip->connTimeout > 0) {
				(void) alarm(0);
				(void) signal(SIGALRM, (FTPSigProc) osigalrm);
			}
			if (gGotSig == SIGINT) {
				result = vcip->errNo = kErrConnectMiscErr;
				Error(vcip, kDontPerror, "Connection attempt canceled.\n");
				(void) kill(getpid(), SIGINT);
			} else if (gGotSig == SIGALRM) {
				result = vcip->errNo = kErrConnectRetryableErr;
				Error(vcip, kDontPerror, "Connection attempt timed-out.\n");
				(void) kill(getpid(), SIGALRM);
			} else {
				result = vcip->errNo = kErrConnectMiscErr;
				Error(vcip, kDontPerror, "Connection attempt failed due to an unexpected signal (%d).\n", gGotSig);
			}
			return (result);
		} else  {
			err = connect(sockfd, (struct sockaddr *) &cip->servCtlAddr,
				      (int) sizeof (cip->servCtlAddr));
			if (cip->connTimeout > 0) {
				(void) alarm(0);
				(void) signal(SIGALRM, (FTPSigProc) osigalrm);
			}
			(void) signal(SIGINT, (FTPSigProc) osigint);
		}

		if (err < 0) {
			oerrno = errno;
			(void) closesocket(sockfd);
			errno = oerrno;
			sockfd = -1;
		}
#endif	/* NO_SIGNALS */
	} else {
		/* We can try each address in the list.  We'll quit when we
		 * run out of addresses to try or get a successful connection.
		 */
		for (curaddr = hp->h_addr_list; *curaddr != NULL; curaddr++) {
			if ((sockfd = socket(cip->servCtlAddr.sin_family, SOCK_STREAM, 0)) < 0) {
				Error(cip, kDoPerror, "Could not get a socket.\n");
				cip->errNo = kErrNewStreamSocket;
				return (kErrNewStreamSocket);
			}
			/* This could overwrite the address field in the structure,
			 * but this is okay because the structure has a junk field
			 * just for this purpose.
			 */
			(void) memcpy(&cip->servCtlAddr.sin_addr, *curaddr, (size_t) hp->h_length);

			/* This doesn't do anything if you left these
			 * at their defaults (zero).  Otherwise it
			 * tries to set the buffer size to the
			 * size specified.
			 */
			(void) SetSockBufSize(sockfd, cip->ctrlSocketRBufSize, cip->ctrlSocketSBufSize);

#ifdef NO_SIGNALS
			err = SConnect(sockfd, &cip->servCtlAddr, (int) cip->connTimeout);

			if (err == 0)
				break;
			oerrno = errno;
			(void) SClose(sockfd, 3);
			errno = oerrno;
			sockfd = -1;
#else	/* NO_SIGNALS */

			osigint = (volatile FTPSigProc) signal(SIGINT, CancelConnect);
			if (cip->connTimeout > 0) {
				osigalrm = (volatile FTPSigProc) signal(SIGALRM, CancelConnect);
				(void) alarm(cip->connTimeout);
			}

			vcip = cip;
#ifdef HAVE_SIGSETJMP
			sj = sigsetjmp(gCancelConnectJmp, 1);
#else
			sj = setjmp(gCancelConnectJmp);
#endif	/* HAVE_SIGSETJMP */

			if (sj != 0) {
				/* Interrupted by a signal. */
				(void) closesocket(sockfd);
				(void) signal(SIGINT, (FTPSigProc) osigint);
				if (vcip->connTimeout > 0) {
					(void) alarm(0);
					(void) signal(SIGALRM, (FTPSigProc) osigalrm);
				}
				if (gGotSig == SIGINT) {
					result = vcip->errNo = kErrConnectMiscErr;
					Error(vcip, kDontPerror, "Connection attempt canceled.\n");
					(void) kill(getpid(), SIGINT);
				} else if (gGotSig == SIGALRM) {
					result = vcip->errNo = kErrConnectRetryableErr;
					Error(vcip, kDontPerror, "Connection attempt timed-out.\n");
					(void) kill(getpid(), SIGALRM);
				} else {
					result = vcip->errNo = kErrConnectMiscErr;
					Error(vcip, kDontPerror, "Connection attempt failed due to an unexpected signal (%d).\n", gGotSig);
				}
				return (result);
			} else {
				err = connect(sockfd, (struct sockaddr *) &cip->servCtlAddr,
					      (int) sizeof (cip->servCtlAddr));
				if (cip->connTimeout > 0) {
					(void) alarm(0);
					(void) signal(SIGALRM, (FTPSigProc) osigalrm);
				}
				(void) signal(SIGINT, (FTPSigProc) osigint);
			}

			if (err == 0)
				break;
			oerrno = errno;
			(void) closesocket(sockfd);
			errno = oerrno;
			sockfd = -1;
#endif /* NO_SIGNALS */
		}
	}
	
	if (err < 0) {
		/* Could not connect.  Close up shop and go home. */

		/* If possible, tell the caller if they should bother
		 * calling back later.
		 */
		switch (errno) {
#ifdef ENETDOWN
			case ENETDOWN:
#elif defined(WSAENETDOWN)
			case WSAENETDOWN:
#endif
#ifdef ENETUNREACH
			case ENETUNREACH:
#elif defined(WSAENETUNREACH)
			case WSAENETUNREACH:
#endif
#ifdef ECONNABORTED
			case ECONNABORTED:
#elif defined(WSAECONNABORTED)
			case WSAECONNABORTED:
#endif
#ifdef ETIMEDOUT
			case ETIMEDOUT:
#elif defined(WSAETIMEDOUT)
			case WSAETIMEDOUT:
#endif
#ifdef EHOSTDOWN
			case EHOSTDOWN:
#elif defined(WSAEHOSTDOWN)
			case WSAEHOSTDOWN:
#endif
#ifdef ECONNRESET
			case ECONNRESET:
#elif defined(WSAECONNRESET)
			case WSAECONNRESET:
#endif
				Error(cip, kDoPerror, "Could not connect to %s -- try again later.\n", fhost);
				result = cip->errNo = kErrConnectRetryableErr;
				break;
#ifdef ECONNREFUSED
			case ECONNREFUSED:
#elif defined(WSAECONNREFUSED)
			case WSAECONNREFUSED:
#endif
				Error(cip, kDoPerror, "Could not connect to %s.\n", fhost);
				result = cip->errNo = kErrConnectRefused;
				break;
			default:
				Error(cip, kDoPerror, "Could not connect to %s.\n", fhost);
				result = cip->errNo = kErrConnectMiscErr;
		}
		goto fatal;
	}

	/* Get our end of the socket address for later use. */
	if ((result = GetSocketAddress(cip, sockfd, &cip->ourCtlAddr)) < 0)
		goto fatal;

#ifdef SO_OOBINLINE
	/* We want Out-of-band data to appear in the regular stream,
	 * since we can handle TELNET.
	 */
	(void) SetInlineOutOfBandData(cip, sockfd);
#endif
	(void) SetKeepAlive(cip, sockfd);
	(void) SetLinger(cip, sockfd, 0);	/* Don't need it for ctrl. */

#if defined(IP_TOS) && defined(IPTOS_LOWDELAY)
	/* Control connection is somewhat interactive, so quick response
	 * is desired.
	 */
	(void) SetTypeOfService(cip, sockfd, IPTOS_LOWDELAY);
#endif

#ifdef NO_SIGNALS
	cip->ctrlSocketR = sockfd;
	cip->ctrlSocketW = sockfd;
	cip->cout = NULL;
	cip->cin = NULL;
	sock2fd = kClosedFileDescriptor;

	if (InitSReadlineInfo(&cip->ctrlSrl, sockfd, cip->srlBuf, sizeof(cip->srlBuf), (int) cip->ctrlTimeout, 1) < 0) {
		result = kErrFdopenW;
		cip->errNo = kErrFdopenW;
		Error(cip, kDoPerror, "Could not fdopen.\n");
		goto fatal;
	}
#else	/* NO_SIGNALS */
	if ((sock2fd = dup(sockfd)) < 0) {
		result = kErrDupSocket;
		cip->errNo = kErrDupSocket;
		Error(cip, kDoPerror, "Could not duplicate a file descriptor.\n");
		goto fatal;
	}

	/* Now setup the FILE pointers for use with the Std I/O library
	 * routines.
	 */
	if ((cip->cin = fdopen(sockfd, "r")) == NULL) {
		result = kErrFdopenR;
		cip->errNo = kErrFdopenR;
		Error(cip, kDoPerror, "Could not fdopen.\n");
		goto fatal;
	}

	if ((cip->cout = fdopen(sock2fd, "w")) == NULL) {
		result = kErrFdopenW;
		cip->errNo = kErrFdopenW;
		Error(cip, kDoPerror, "Could not fdopen.\n");
		CloseFile(&cip->cin);
		sockfd = kClosedFileDescriptor;
		goto fatal;
	}

	cip->ctrlSocketR = sockfd;
	cip->ctrlSocketW = sockfd;

	/* We'll be reading and writing lines, so use line buffering.  This
	 * is necessary since the stdio library will use full buffering
	 * for all streams not associated with the tty.
	 */
#ifdef HAVE_SETLINEBUF
	setlinebuf(cip->cin);
	setlinebuf(cip->cout);
#else
	(void) SETVBUF(cip->cin, NULL, _IOLBF, (size_t) BUFSIZ);
	(void) SETVBUF(cip->cout, NULL, _IOLBF, (size_t) BUFSIZ);
#endif
#endif	/* NO_SIGNALS */

#ifdef HAVE_INET_NTOP	/* Mostly to workaround bug in IRIX 6.5's inet_ntoa */
	(void) memset(cip->ip, 0, sizeof(cip->ip));
	(void) inet_ntop(AF_INET, &cip->servCtlAddr.sin_addr, cip->ip, sizeof(cip->ip) - 1);
#else
	(void) STRNCPY(cip->ip, inet_ntoa(cip->servCtlAddr.sin_addr));
#endif
	if ((hp == NULL) || (hp->h_name == NULL))
		(void) STRNCPY(cip->actualHost, fhost);
	else
		(void) STRNCPY(cip->actualHost, (char *) hp->h_name);

	/* Read the startup message from the server. */	
	rp = InitResponse();
	if (rp == NULL) {
		Error(cip, kDontPerror, "Malloc failed.\n");
		cip->errNo = kErrMallocFailed;
		result = cip->errNo;
		goto fatal;
	}

	result = GetResponse(cip, rp);
	if ((result < 0) && (rp->msg.first == NULL)) {
		goto fatal;
	}
	if (rp->msg.first != NULL) {
		cip->serverType = kServerTypeUnknown;
		srvr = NULL;
		firstLine = rp->msg.first->line;
		secondLine = NULL;
		if (rp->msg.first->next != NULL)
			secondLine = rp->msg.first->next->line;
		
		if (strstr(firstLine, "Version wu-") != NULL) {
			cip->serverType = kServerTypeWuFTPd;
			srvr = "wu-ftpd";
		} else if (strstr(firstLine, "NcFTPd") != NULL) {
			cip->serverType = kServerTypeNcFTPd;
			srvr = "NcFTPd Server";
		} else if (STRNEQ("ProFTPD", firstLine, 7)) {
			cip->serverType = kServerTypeProFTPD;
			srvr = "ProFTPD";
		} else if (strstr(firstLine, "Microsoft FTP Service") != NULL) {
			cip->serverType = kServerTypeMicrosoftFTP;
			srvr = "Microsoft FTP Service";
		} else if (strstr(firstLine, "(NetWare ") != NULL) {
			cip->serverType = kServerTypeNetWareFTP;
			srvr = "NetWare FTP Service";
		} else if (STRNEQ("WFTPD", firstLine, 5)) {
			cip->serverType = kServerTypeWFTPD;
			srvr = "WFTPD";
		} else if (STRNEQ("Serv-U FTP", firstLine, 10)) {
			cip->serverType = kServerTypeServ_U;
			srvr = "Serv-U FTP-Server";
		} else if (strstr(firstLine, "VFTPD") != NULL) {
			cip->serverType = kServerTypeVFTPD;
			srvr = "VFTPD";
		} else if (STRNEQ("FTP-Max", firstLine, 7)) {
			cip->serverType = kServerTypeFTP_Max;
			srvr = "FTP-Max";
		} else if (strstr(firstLine, "Roxen") != NULL) {
			cip->serverType = kServerTypeRoxen;
			srvr = "Roxen";
		} else if (strstr(firstLine, "WS_FTP") != NULL) {
			cip->serverType = kServerTypeWS_FTP;
			srvr = "WS_FTP Server";
		} else if ((secondLine != NULL) && (strstr(secondLine, "WarFTP") != NULL)) {
			cip->serverType = kServerTypeWarFTPd;
			srvr = "WarFTPd";
		}

		if (srvr != NULL)
			PrintF(cip, "Remote server is running %s.\n", srvr);

		/* Do the application's connect message callback, if present. */
		if ((cip->onConnectMsgProc != 0) && (rp->codeType < 4))
			(*cip->onConnectMsgProc)(cip, rp);
	}

	if (rp->codeType >= 4) {
		/* They probably hung up on us right away.  That's too bad,
		 * but we can tell the caller that they can call back later
		 * and try again.
		 */
		DoneWithResponse(cip, rp);
		result = kErrConnectRetryableErr;
		Error(cip, kDontPerror, "Server hungup immediately after connect.\n");
		cip->errNo = kErrConnectRetryableErr;
		goto fatal;
	}
	if (result < 0)		/* Some other error occurred during connect message */
		goto fatal;
	cip->connected = 1;
	DoneWithResponse(cip, rp);
	return (kNoErr);
	
fatal:
	if (sockfd > 0)
		(void) closesocket(sockfd);
	if (sock2fd > 0)
		(void) closesocket(sock2fd);		
	CloseFile(&cip->cin);
	CloseFile(&cip->cout);
	cip->ctrlSocketR = kClosedFileDescriptor;
	cip->ctrlSocketW = kClosedFileDescriptor;
	return (result);
}	/* OpenControlConnection */




void
CloseDataConnection(const FTPCIPtr cip)
{
	if (cip->dataSocket != kClosedFileDescriptor) {
#ifdef NO_SIGNALS
		SClose(cip->dataSocket, 3);
#else	/* NO_SIGNALS */
		if (cip->xferTimeout > 0)
			(void) alarm(cip->xferTimeout);
		(void) closesocket(cip->dataSocket);
		if (cip->xferTimeout > 0)
			(void) alarm(0);
#endif	/* NO_SIGNALS */
		cip->dataSocket = kClosedFileDescriptor;
	}
	memset(&cip->ourDataAddr, 0, sizeof(cip->ourDataAddr));
	memset(&cip->servDataAddr, 0, sizeof(cip->servDataAddr));
}	/* CloseDataConnection */




int
SetStartOffset(const FTPCIPtr cip, longest_int restartPt)
{
	ResponsePtr rp;
	int result;

	if (restartPt != (longest_int) 0) {
		rp = InitResponse();
		if (rp == NULL) {
			Error(cip, kDontPerror, "Malloc failed.\n");
			cip->errNo = kErrMallocFailed;
			return (cip->errNo);
		}

		/* Force reset to offset zero. */
		if (restartPt == (longest_int) -1)
			restartPt = (longest_int) 0;
#ifdef PRINTF_LONG_LONG
		result = RCmd(cip, rp,
		"REST " PRINTF_LONG_LONG,
		restartPt);
#else
		result = RCmd(cip, rp, "REST %ld", (long) restartPt);
#endif

		if (result < 0) {
			return (result);
		} else if (result == 3) {
			cip->hasREST = kCommandAvailable;
			DoneWithResponse(cip, rp);
		} else if (UNIMPLEMENTED_CMD(rp->code)) {
			cip->hasREST = kCommandNotAvailable;
			DoneWithResponse(cip, rp);
			cip->errNo = kErrSetStartPoint;
			return (kErrSetStartPoint);
		} else {
			DoneWithResponse(cip, rp);
			cip->errNo = kErrSetStartPoint;
			return (kErrSetStartPoint);
		}
	}
	return (0);
}	/* SetStartOffset */



static int
SendPort(const FTPCIPtr cip, struct sockaddr_in *saddr)
{
	char *a, *p;
	int result;
	ResponsePtr rp;

	rp = InitResponse();
	if (rp == NULL) {
		Error(cip, kDontPerror, "Malloc failed.\n");
		cip->errNo = kErrMallocFailed;
		return (cip->errNo);
	}

	/* These will point to data in network byte order. */
	a = (char *) &saddr->sin_addr;
	p = (char *) &saddr->sin_port;
#define UC(x) (int) (((int) x) & 0xff)

	/* Need to tell the other side which host (the address) and
	 * which process (port) on that host to send data to.
	 */
	result = RCmd(cip, rp, "PORT %d,%d,%d,%d,%d,%d",
		UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));

	if (result < 0) {
		return (result);
	} else if (result != 2) {
		/* A 500'ish response code means the PORT command failed. */
		DoneWithResponse(cip, rp);
		cip->errNo = kErrPORTFailed;
		return (cip->errNo);
	}
	DoneWithResponse(cip, rp);
	return (kNoErr);
}	/* SendPort */




static int
Passive(const FTPCIPtr cip, struct sockaddr_in *saddr, int *weird)
{
	ResponsePtr rp;
	int i[6], j;
	unsigned char n[6];
	char *cp;
	int result;

	rp = InitResponse();
	if (rp == NULL) {
		Error(cip, kDontPerror, "Malloc failed.\n");
		cip->errNo = kErrMallocFailed;
		return (cip->errNo);
	}

	result = RCmd(cip, rp, "PASV");
	if (result < 0)
		goto done;

	if (rp->codeType != 2) {
		/* Didn't understand or didn't want passive port selection. */
		cip->errNo = result = kErrPASVFailed;
		goto done;
	}

	/* The other side returns a specification in the form of
	 * an internet address as the first four integers (each
	 * integer stands for 8-bits of the real 32-bit address),
	 * and two more integers for the port (16-bit port).
	 *
	 * It should give us something like:
	 * "Entering Passive Mode (129,93,33,1,10,187)", so look for
	 * digits with sscanf() starting 24 characters down the string.
	 */
	for (cp = rp->msg.first->line; ; cp++) {
		if (*cp == '\0') {
			Error(cip, kDontPerror, "Cannot parse PASV response: %s\n", rp->msg.first->line);
			goto done;
		}
		if (isdigit((int) *cp))
			break;
	}

	if (sscanf(cp, "%d,%d,%d,%d,%d,%d",
			&i[0], &i[1], &i[2], &i[3], &i[4], &i[5]) != 6) {
		Error(cip, kDontPerror, "Cannot parse PASV response: %s\n", rp->msg.first->line);
		goto done;
	}

	for (j=0, *weird = 0; j<6; j++) {
		/* Some ftp servers return bogus port octets, such as
		 * boombox.micro.umn.edu.  Let the caller know if we got a
		 * weird looking octet.
		 */
		if ((i[j] < 0) || (i[j] > 255))
			*weird = *weird + 1;
		n[j] = (unsigned char) (i[j] & 0xff);
	}

	(void) memcpy(&saddr->sin_addr, &n[0], (size_t) 4);
	(void) memcpy(&saddr->sin_port, &n[4], (size_t) 2);

	result = kNoErr;
done:
	DoneWithResponse(cip, rp);
	return (result);
}	/* Passive */




static int
BindToEphemeralPortNumber(int sockfd, struct sockaddr_in *addrp, int ephemLo, int ephemHi)
{
	int i;
	int result;
	int rangesize;
	unsigned short port;

	addrp->sin_family = AF_INET;
	if (((int) ephemLo == 0) || ((int) ephemLo >= (int) ephemHi)) {
		/* Do it the normal way.  System will
		 * pick one, typically in the range
		 * of 1024-4999.
		 */
		addrp->sin_port = 0;	/* Let system pick one. */

		result = bind(sockfd, (struct sockaddr *) addrp, sizeof(struct sockaddr_in));
	} else {
		rangesize = (int) ((int) ephemHi - (int) ephemLo);
		result = 0;
		for (i=0; i<10; i++) {
			port = (unsigned short) (((int) rand() % rangesize) + (int) ephemLo);
			addrp->sin_port = port;

			result = bind(sockfd, (struct sockaddr *) addrp, sizeof(struct sockaddr_in));
			if (result == 0)
				break;
			if ((errno != 999)
				/* This next line is just fodder to
				 * shut the compiler up about variable
				 * not being used.
				 */
				&& (gCopyright[0] != '\0'))
				break;
		}
	}
	return (result);
}	/* BindToEphemeralPortNumber */




int
OpenDataConnection(const FTPCIPtr cip, int mode)
{
	int dataSocket;
	int weirdPort;
	int result;

	/* Before we can transfer any data, and before we even ask the
	 * remote server to start transferring via RETR/NLST/etc, we have
	 * to setup the connection.
	 */

tryPort2:
	weirdPort = 0;
	result = 0;
	CloseDataConnection(cip);	/* In case we didn't before... */

	dataSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (dataSocket < 0) {
		Error(cip, kDoPerror, "Could not get a data socket.\n");
		result = kErrNewStreamSocket;
		cip->errNo = kErrNewStreamSocket;
		return result;
	}

	/* This doesn't do anything if you left these
	 * at their defaults (zero).  Otherwise it
	 * tries to set the buffer size to the
	 * size specified.
	 */
	(void) SetSockBufSize(dataSocket, cip->dataSocketRBufSize, cip->dataSocketSBufSize);

	if ((cip->hasPASV == kCommandNotAvailable) || (mode == kSendPortMode)) {
tryPort:
		cip->ourDataAddr = cip->ourCtlAddr;
		cip->ourDataAddr.sin_family = AF_INET;

#ifdef HAVE_LIBSOCKS
		cip->ourDataAddr.sin_port = 0;
		if (Rbind(dataSocket, (struct sockaddr *) &cip->ourDataAddr,
			(int) sizeof (cip->ourDataAddr),
			cip->servCtlAddr.sin_addr.s_addr) < 0) 
#else
		if (BindToEphemeralPortNumber(dataSocket, &cip->ourDataAddr, (int) cip->ephemLo, (int) cip->ephemHi) < 0)
#endif
		{
			Error(cip, kDoPerror, "Could not bind the data socket");
			result = kErrBindDataSocket;
			cip->errNo = kErrBindDataSocket;
			goto bad;
		}
	
		/* Need to do this so we can figure out which port the system
		 * gave to us.
		 */
		if ((result = GetSocketAddress(cip, dataSocket, &cip->ourDataAddr)) < 0)
			goto bad;
	
		if (listen(dataSocket, 1) < 0) {
			Error(cip, kDoPerror, "listen failed");
			result = kErrListenDataSocket;
			cip->errNo = kErrListenDataSocket;
			goto bad;
		}
	
		if ((result = SendPort(cip, &cip->ourDataAddr)) < 0)
			goto bad;
	
		cip->dataPortMode = kSendPortMode;
	} else {
		/* Passive mode.  Let the other side decide where to send. */
		
		cip->servDataAddr = cip->servCtlAddr;
		cip->servDataAddr.sin_family = AF_INET;
		cip->ourDataAddr = cip->ourCtlAddr;
		cip->ourDataAddr.sin_family = AF_INET;

		if (Passive(cip, &cip->servDataAddr, &weirdPort) < 0) {
			Error(cip, kDontPerror, "Passive mode refused.\n");
			cip->hasPASV = kCommandNotAvailable;
			
			/* We can try using regular PORT commands, which are required
			 * by all FTP protocol compliant programs, if you said so.
			 *
			 * We don't do this automatically, because if your host
			 * is running a firewall you (probably) do not want SendPort
			 * FTP for security reasons.
			 */
			if (mode == kFallBackToSendPortMode)
				goto tryPort;
			result = kErrPassiveModeFailed;
			cip->errNo = kErrPassiveModeFailed;
			goto bad;
		}

#ifdef HAVE_LIBSOCKS
		cip->ourDataAddr.sin_port = 0;
		if (Rbind(dataSocket, (struct sockaddr *) &cip->ourDataAddr,
			(int) sizeof (cip->ourDataAddr),
			cip->servCtlAddr.sin_addr.s_addr) < 0) 
#else
		if (BindToEphemeralPortNumber(dataSocket, &cip->ourDataAddr, (int) cip->ephemLo, (int) cip->ephemHi) < 0)
#endif
		{
			Error(cip, kDoPerror, "Could not bind the data socket");
			result = kErrBindDataSocket;
			cip->errNo = kErrBindDataSocket;
			goto bad;
		}

#ifdef NO_SIGNALS
		result = SConnect(dataSocket, &cip->servDataAddr, (int) cip->connTimeout);
#else	/* NO_SIGNALS */
		if (cip->connTimeout > 0)
			(void) alarm(cip->connTimeout);

		result = connect(dataSocket, (struct sockaddr *) &cip->servDataAddr, (int) sizeof(cip->servDataAddr));
		if (cip->connTimeout > 0)
			(void) alarm(0);
#endif	/* NO_SIGNALS */

#ifdef NO_SIGNALS
		if (result == kTimeoutErr) {
			if (mode == kFallBackToSendPortMode) {
				Error(cip, kDontPerror, "Data connection timed out.\n");
				Error(cip, kDontPerror, "Falling back to PORT instead of PASV mode.\n");
				(void) closesocket(dataSocket);
				cip->hasPASV = kCommandNotAvailable;
				goto tryPort2;
			}
			Error(cip, kDontPerror, "Data connection timed out.\n");
			result = kErrConnectDataSocket;
			cip->errNo = kErrConnectDataSocket;
		} else
#endif	/* NO_SIGNALS */

		if (result < 0) {
#ifdef ECONNREFUSED
			if ((weirdPort > 0) && (errno == ECONNREFUSED)) {
#elif defined(WSAECONNREFUSED)
			if ((weirdPort > 0) && (errno == WSAECONNREFUSED)) {
#endif
				Error(cip, kDontPerror, "Server sent back a bogus port number.\nI will fall back to PORT instead of PASV mode.\n");
				if (mode == kFallBackToSendPortMode) {
					(void) closesocket(dataSocket);
					cip->hasPASV = kCommandNotAvailable;
					goto tryPort2;
				}
				result = kErrServerSentBogusPortNumber;
				cip->errNo = kErrServerSentBogusPortNumber;
				goto bad;
			}
			if (mode == kFallBackToSendPortMode) {
				Error(cip, kDoPerror, "connect failed.\n");
				Error(cip, kDontPerror, "Falling back to PORT instead of PASV mode.\n");
				(void) closesocket(dataSocket);
				cip->hasPASV = kCommandNotAvailable;
				goto tryPort2;
			}
			Error(cip, kDoPerror, "connect failed.\n");
			result = kErrConnectDataSocket;
			cip->errNo = kErrConnectDataSocket;
			goto bad;
		}
	
		/* Need to do this so we can figure out which port the system
		 * gave to us.
		 */
		if ((result = GetSocketAddress(cip, dataSocket, &cip->ourDataAddr)) < 0)
			goto bad;

		cip->dataPortMode = kPassiveMode;
		cip->hasPASV = kCommandAvailable;
	}

	(void) SetLinger(cip, dataSocket, 1);
	(void) SetKeepAlive(cip, dataSocket);

#if defined(IP_TOS) && defined(IPTOS_THROUGHPUT)
	/* Data connection is a non-interactive data stream, so
	 * high throughput is desired, at the expense of low
	 * response time.
	 */
	(void) SetTypeOfService(cip, dataSocket, IPTOS_THROUGHPUT);
#endif

	cip->dataSocket = dataSocket;
	return (0);
bad:
	(void) closesocket(dataSocket);
	return (result);
}	/* OpenDataConnection */




int
AcceptDataConnection(const FTPCIPtr cip)
{
	int newSocket;
#ifndef NO_SIGNALS
	int len;
#endif
	unsigned short remoteDataPort;
	unsigned short remoteCtrlPort;

	/* If we did a PORT, we have some things to finish up.
	 * If we did a PASV, we're ready to go.
	 */
	if (cip->dataPortMode == kSendPortMode) {
		/* Accept will give us back the server's data address;  at the
		 * moment we don't do anything with it though.
		 */
		memset(&cip->servDataAddr, 0, sizeof(cip->servDataAddr));

#ifdef NO_SIGNALS
		newSocket = SAccept(cip->dataSocket, &cip->servDataAddr, (int) cip->connTimeout);
#else	/* NO_SIGNALS */
		len = (int) sizeof(cip->servDataAddr);
		if (cip->connTimeout > 0)
			(void) alarm(cip->connTimeout);
		newSocket = accept(cip->dataSocket, (struct sockaddr *) &cip->servDataAddr, &len);
		if (cip->connTimeout > 0)
			(void) alarm(0);
#endif	/* NO_SIGNALS */

		(void) closesocket(cip->dataSocket);
		if (newSocket < 0) {
			Error(cip, kDoPerror, "Could not accept a data connection.\n");
			cip->dataSocket = kClosedFileDescriptor;
			cip->errNo = kErrAcceptDataSocket;
			return (kErrAcceptDataSocket);
		}

		if (cip->require20 != 0) {
			remoteDataPort = ntohs(cip->servDataAddr.sin_port);
			remoteCtrlPort = ntohs(cip->servCtlAddr.sin_port);
			if ((int) remoteDataPort != ((int) remoteCtrlPort - 1)) {
				Error(cip, kDontPerror, "Data connection did not originate on correct port!\n");
				(void) closesocket(newSocket);
				cip->dataSocket = kClosedFileDescriptor;
				cip->errNo = kErrAcceptDataSocket;
				return (kErrAcceptDataSocket);
			} else if (memcmp(&cip->servDataAddr.sin_addr.s_addr, &cip->servCtlAddr.sin_addr.s_addr, sizeof(cip->servDataAddr.sin_addr.s_addr)) != 0) {
				Error(cip, kDontPerror, "Data connection did not originate from remote server!\n");
				(void) closesocket(newSocket);
				cip->dataSocket = kClosedFileDescriptor;
				cip->errNo = kErrAcceptDataSocket;
				return (kErrAcceptDataSocket);
			}
		}
		
		cip->dataSocket = newSocket;
	}

	return (0);
}	/* AcceptDataConnection */




void
HangupOnServer(const FTPCIPtr cip)
{
	/* Since we want to close both sides of the connection for each
	 * socket, we can just have them closed with close() instead of
	 * using shutdown().
	 */
	CloseControlConnection(cip);
	CloseDataConnection(cip);
}	/* HangupOnServer */




void
SendTelnetInterrupt(const FTPCIPtr cip)
{
	char msg[4];

	/* 1. User system inserts the Telnet "Interrupt Process" (IP) signal
	 *    in the Telnet stream.
	 */

	if (cip->cout != NULL)
		(void) fflush(cip->cout);
	
	msg[0] = (char) (unsigned char) IAC;
	msg[1] = (char) (unsigned char) IP;
	(void) send(cip->ctrlSocketW, msg, 2, 0);

	/* 2. User system sends the Telnet "Sync" signal. */
#if 1
	msg[0] = (char) (unsigned char) IAC;
	msg[1] = (char) (unsigned char) DM;
	if (send(cip->ctrlSocketW, msg, 2, MSG_OOB) != 2)
		Error(cip, kDoPerror, "Could not send an urgent message.\n");
#else
	/* "Send IAC in urgent mode instead of DM because UNIX places oob mark
	 * after urgent byte rather than before as now is protocol," says
	 * the BSD ftp code.
	 */
	msg[0] = (char) (unsigned char) IAC;
	if (send(cip->ctrlSocketW, msg, 1, MSG_OOB) != 1)
		Error(cip, kDoPerror, "Could not send an urgent message.\n");
	(void) fprintf(cip->cout, "%c", DM);
	(void) fflush(cip->cout);
#endif
}	/* SendTelnetInterrupt */

/* eof FTP.c */
