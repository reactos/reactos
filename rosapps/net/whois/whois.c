/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * 8/1/97 - Ted Felix <tfelix@fred.net>
 *          Ported to Win32 from 4.4-BSDLITE2 from wcarchive.
 *          Added WSAStartup()/WSACleanup() and switched from the
 *          more convenient fdopen()/fprintf() to send()/recv().
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)whois.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/types.h>
#include <winsock2.h>
/* #include <sys/socket.h> */
/* #include <netinet/in.h> */
/* #include <netdb.h> */
#include <stdio.h>

/* #include <various.h> */
#include <getopt.h>
#include <io.h>

#define	NICHOST	"whois.internic.net"

void usage();
void leave(int iExitCode);

int main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	char ch;
	struct sockaddr_in sin;
	struct hostent *hp;
	struct servent *sp;
	int s;
	char *host;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	host = NICHOST;
	while ((ch = (char)getopt(argc, argv, "h:")) != EOF)
		switch((char)ch) {
		case 'h':
			host = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!argc)
		usage();

	/* Start winsock */ 
	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		/* Tell the user that we couldn't find a usable */
		/* WinSock DLL.                                 */
		perror("whois: WSAStartup failed");
		leave(1);
	}

	hp = gethostbyname(host);
	if (hp == NULL) {
		(void)fprintf(stderr, "whois: %s: ", host);
		leave(1);
	}
	host = hp->h_name;

	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		perror("whois: socket");
		leave(1);
	}

	bzero(/*(caddr_t)*/&sin, sizeof (sin));
	sin.sin_family = hp->h_addrtype;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("whois: bind");
		leave(1);
	}

	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sp = getservbyname("whois", "tcp");
	if (sp == NULL) {
		(void)fprintf(stderr, "whois: whois/tcp: unknown service\n");
		leave(1);
	}

	sin.sin_port = sp->s_port;

	/* have network connection; identify the host connected with */
	(void)printf("[%s]\n", hp->h_name);

	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "whois: connect error = %d\n", WSAGetLastError());
		leave(1);
	}

	/* WinSock doesn't allow using a socket as a file descriptor. */
	/* Have to use send() and recv().  whois will drop connection. */

	/* For each request */
	while (argc-- > 1)
	{
		/* Send the request */
		send(s, *argv, strlen(*argv), 0);
		send(s, " ", 1, 0);
		argv++;
	}
	/* Send the last request */
	send(s, *argv, strlen(*argv), 0);
	send(s, "\r\n", 2, 0);

	/* Receive anything and print it */
	while (recv(s, &ch, 1, 0) == 1)
		putchar(ch);

	leave(0);
}

void usage()
{
	(void)fprintf(stderr, "usage: whois [-h hostname] name ...\n");
	leave(1);
}

void leave(int iExitCode)
{
	WSACleanup();
	exit(iExitCode);
}
