/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tony Nardo of the Johns Hopkins University/Applied Physics Lab.
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
 */

#ifndef lint
static char sccsid[] = "@(#)net.c	8.4 (Berkeley) 4/28/95";
#endif /* not lint */

#include <sys/types.h>
#include <winsock2.h>
#include "unistd.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "various.h"

void
netfinger(char *name)
{
	extern int lflag;
	char c, lastc;
	struct in_addr defaddr;
	struct hostent *hp, def;
	struct servent *sp;
	struct sockaddr_in sin;
	int s;
	char *alist[1], *host;

	/* If this is a local request */
	if (!(host = rindex(name, '@')))
		return;

	*host++ = NULL;
	if (isdigit(*host) && (defaddr.s_addr = inet_addr(host)) != -1) {
		def.h_name = host;
		def.h_addr_list = alist;
		def.h_addr = (char *)&defaddr;
		def.h_length = sizeof(struct in_addr);
		def.h_addrtype = AF_INET;
		def.h_aliases = 0;
		hp = &def;
	} else if (!(hp = gethostbyname(host))) {
		(void)fprintf(stderr,
		    "finger: unknown host: %s\n", host);
		return;
	}
	if (!(sp = getservbyname("finger", "tcp"))) {
		(void)fprintf(stderr, "finger: tcp/finger: unknown service\n");
		return;
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = sp->s_port;
	if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
		perror("finger: socket");
		return;
	}

	/* have network connection; identify the host connected with */
	(void)printf("[%s]\n", hp->h_name);
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "finger: connect rc = %d", WSAGetLastError());
		(void)close(s);
		return;
	}

	/* -l flag for remote fingerd  */
	if (lflag)
		send(s, "/W ", 3, 0);
	/* send the name followed by <CR><LF> */
	send(s, name, strlen(name), 0);
	send(s, "\r\n", 2, 0);

	/*
	 * Read from the remote system; once we're connected, we assume some
	 * data.  If none arrives, we hang until the user interrupts.
	 *
	 * If we see a <CR> or a <CR> with the high bit set, treat it as
	 * a newline; if followed by a newline character, only output one
	 * newline.
	 *
	 * Otherwise, all high bits are stripped; if it isn't printable and
	 * it isn't a space, we can simply set the 7th bit.  Every ASCII
	 * character with bit 7 set is printable.
	 */ 
	lastc = 0;
	while (recv(s, &c, 1, 0) == 1) {
		c &= 0x7f;
		if (c == 0x0d) {
			if (lastc == '\r')	/* ^M^M - skip dupes */
				continue;
			c = '\n';
			lastc = '\r';
		} else {
			if (!isprint(c) && !isspace(c))
				c |= 0x40;
			if (lastc != '\r' || c != '\n')
				lastc = c;
			else {
				lastc = '\n';
				continue;
			}
		}
		putchar(c);
	}
	if (lastc != '\n')
		putchar('\n');
	putchar('\n');
}
