/*
 * arpa/inet.h
 *
 * definitions for internet operations. Based on the Single UNIX(r)
 * Specification, Version 2
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __ARPA_INET_H_INCLUDED__
#define __ARPA_INET_H_INCLUDED__

#ifdef __PSXDLL__

/* headers for internal usage by psxdll.dll and ReactOS */
#include <psxdll/netinet/in.h>
#include <psxdll/inttypes.h>

#else /* ! __PSXDLL__ */

/* standard POSIX headers */
#include <netinet/in.h>
#include <inttypes.h>

#endif

/* types */

/* constants */

/* prototypes */
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

in_addr_t      inet_addr(const char *cp);
in_addr_t      inet_lnaof(struct in_addr in);
struct in_addr inet_makeaddr(in_addr_t net, in_addr_t lna);
in_addr_t      inet_netof(struct in_addr in);
in_addr_t      inet_network(const char *cp);
char          *inet_ntoa(struct in_addr in);

/* macros */

#endif /* __ARPA_INET_H_INCLUDED__ */

/* EOF */

