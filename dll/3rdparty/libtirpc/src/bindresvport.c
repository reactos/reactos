/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//#include <sys/cdefs.h>

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 * Portions Copyright(C) 1996, Jason Downs.  All rights reserved.
 */

#include <wintirpc.h>
#include <sys/types.h>
//#include <sys/socket.h>

//#include <netinet/in.h>

#include <errno.h>
#include <string.h>
//#include <unistd.h>

#include <rpc/rpc.h>

/*
 * Bind a socket to a privileged IP port
 */
int
bindresvport(sd, sin)
        SOCKET sd;
        struct sockaddr_in *sin;
{
        return bindresvport_sa(sd, (struct sockaddr *)sin);
}

#ifdef __linux__

#define STARTPORT 600
#define LOWPORT 512
#define ENDPORT (IPPORT_RESERVED - 1)
#define NPORTS  (ENDPORT - STARTPORT + 1)

int
bindresvport_sa(sd, sa)
        int sd;
        struct sockaddr *sa;
{
        int res, af;
        struct sockaddr_storage myaddr;
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
	u_int16_t *portp;
	static u_int16_t port;
	static short startport = STARTPORT;
	socklen_t salen;
	int nports = ENDPORT - startport + 1;
	int endport = ENDPORT;
	int i;

        if (sa == NULL) {
                salen = sizeof(myaddr);
                sa = (struct sockaddr *)&myaddr;

                if (getsockname(sd, (struct sockaddr *)&myaddr, &salen) == -1)
                        return -1;      /* errno is correctly set */

                af = myaddr.ss_family;
        } else
                af = sa->sa_family;

        switch (af) {
        case AF_INET:
		sin = (struct sockaddr_in *)sa;
                salen = sizeof(struct sockaddr_in);
                port = ntohs(sin->sin_port);
		portp = &sin->sin_port;
		break;
#ifdef INET6
        case AF_INET6:
		sin6 = (struct sockaddr_in6 *)sa;
                salen = sizeof(struct sockaddr_in6);
                port = ntohs(sin6->sin6_port);
                portp = &sin6->sin6_port;
                break;
#endif
        default:
                errno = EPFNOSUPPORT;
                return (-1);
        }
        sa->sa_family = af;

        if (port == 0) {
                port = (getpid() % NPORTS) + STARTPORT;
        }
        res = -1;
        errno = EADDRINUSE;
		again:
        for (i = 0; i < nports; ++i) {
                *portp = htons(port++);
                 if (port > endport) 
                        port = startport;
                res = bind(sd, sa, salen);
		if (res >= 0 || errno != EADDRINUSE)
	                break;
        }
	if (i == nports && startport != LOWPORT) {
	    startport = LOWPORT;
	    endport = STARTPORT - 1;
	    nports = STARTPORT - LOWPORT;
	    port = LOWPORT + port % (STARTPORT - LOWPORT);
	    goto again;
	}
        return (res);
}

#else
/*----------------------
#if defined(_WIN32)

int
bindresvport_sa(SOCKET sd, struct sockaddr *sa)
{
	fprintf(stderr, "Do-nothing bindresvport_sa!\n");
	return 0;
}
#else
-------------------------*/
#define IP_PORTRANGE 19
#define IP_PORTRANGE_LOW 2

/*
 * Bind a socket to a privileged IP port
 */
int
bindresvport_sa(sd, sa)
	SOCKET sd;
	struct sockaddr *sa;
{
#ifdef IPV6_PORTRANGE
	int old;
#endif
	int error, af;
	struct sockaddr_storage myaddr;
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
	int proto, portrange, portlow;
	u_int16_t *portp;
	socklen_t salen;
#ifdef _WIN32
		WSAPROTOCOL_INFO proto_info;
		int proto_info_size = sizeof(proto_info);
#endif

	if (sa == NULL) {
		salen = sizeof(myaddr);
		sa = (struct sockaddr *)&myaddr;

#ifdef _WIN32
		memset(sa, 0, salen);
		if (error = getsockopt(sd, SOL_SOCKET, SO_PROTOCOL_INFO, (char *)&proto_info, &proto_info_size) == SOCKET_ERROR) {
#ifndef __REACTOS__
			int sockerr = WSAGetLastError();
#endif
			return -1;
		}
		af = proto_info.iAddressFamily;
#else
		if (getsockname(sd, sa, &salen) == -1)
			return -1;	/* errno is correctly set */

		af = sa->sa_family;
		memset(sa, 0, salen);
#endif
	} else
		af = sa->sa_family;

	switch (af) {
	case AF_INET:
		proto = IPPROTO_IP;
		portrange = IP_PORTRANGE;
		portlow = IP_PORTRANGE_LOW;
		sin = (struct sockaddr_in *)sa;
		salen = sizeof(struct sockaddr_in);
		portp = &sin->sin_port;
		break;
#ifdef INET6
	case AF_INET6:
		proto = IPPROTO_IPV6;
#ifdef IPV6_PORTRANGE
		portrange = IPV6_PORTRANGE;
		portlow = IPV6_PORTRANGE_LOW;
#endif
		sin6 = (struct sockaddr_in6 *)sa;
		salen = sizeof(struct sockaddr_in6);
		portp = &sin6->sin6_port;
		break;
#endif /* INET6 */
	default:
		errno = WSAEPFNOSUPPORT;
		return (-1);
	}
	sa->sa_family = (ADDRESS_FAMILY) af;

#ifdef IPV6_PORTRANGE
	if (*portp == 0) {
		socklen_t oldlen = sizeof(old);

		error = getsockopt(sd, proto, portrange, &old, &oldlen);
		if (error < 0)
			return (error);

		error = setsockopt(sd, proto, portrange, &portlow,
		    sizeof(portlow));
		if (error < 0)
			return (error);
	}
#endif

	error = bind(sd, sa, salen);
	if (error) {
#ifndef __REACTOS__
		int err = WSAGetLastError();
#endif
	}

#ifdef IPV6_PORTRANGE
	if (*portp == 0) {
		int saved_errno = errno;

		if (error < 0) {
			if (setsockopt(sd, proto, portrange, &old,
			    sizeof(old)) < 0)
				errno = saved_errno;
			return (error);
		}

		if (sa != (struct sockaddr *)&myaddr) {
			/* Hmm, what did the kernel assign? */
			if (getsockname(sd, sa, &salen) < 0)
				errno = saved_errno;
			return (error);
		}
	}
#endif
	return (error);
}
/*
#endif
*/
#endif
