/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nameser.h

Abstract:

    Definitions for the DNS resolver and nameserver.

Author:

    Mike Massa (mikemas)           Jan 31, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-31-92     created

Notes:

--*/

/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  NAMESER.H
 *
 ******************************************************************/

/*
 *   /usr/projects/tcp/SCCS.rel3/rel/src/include/arpa/0/s.nameser.h
 *  @(#)nameser.h   5.3
 *
 *  Last delta created  14:06:04 3/4/91
 *  This file extracted 11:19:28 3/8/91
 *
 *  Modifications:
 *
 *      GSS 20 Jul 90   New File
 */

/*
 * Copyright (c) 1983, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  @(#)nameser.h   5.24 (Berkeley) 6/1/90
 */

#ifndef _NAMESER_INCLUDED
#define _NAMESER_INCLUDED


/*
 * Define constants based on rfc883
 */
#define PACKETSZ    512     /* maximum packet size */
#define MAXDNAME    256     /* maximum domain name */
#define MAXCDNAME   255     /* maximum compressed domain name */
#define MAXLABEL    63      /* maximum length of domain label */
    /* Number of bytes of fixed size data in query structure */
#define QFIXEDSZ    4
    /* number of bytes of fixed size data in resource record */
#define RRFIXEDSZ   10

/*
 * Internet nameserver port number
 */
#define NAMESERVER_PORT 53

/*
 * Currently defined opcodes
 */
#define QUERY       0x0     /* standard query */
#define IQUERY      0x1     /* inverse query */
#define STATUS      0x2     /* nameserver status query */
/*#define xxx       0x3     /* 0x3 reserved */
    /* non standard */
#define UPDATEA     0x9     /* add resource record */
#define UPDATED     0xa     /* delete a specific resource record */
#define UPDATEDA    0xb     /* delete all nemed resource record */
#define UPDATEM     0xc     /* modify a specific resource record */
#define UPDATEMA    0xd     /* modify all named resource record */

#define ZONEINIT    0xe     /* initial zone transfer */
#define ZONEREF     0xf     /* incremental zone referesh */

/*
 * Currently defined response codes
 */
#ifndef NOERROR
#define NOERROR     0       /* no error */
#endif
#define FORMERR     1       /* format error */
#define SERVFAIL    2       /* server failure */
#define NXDOMAIN    3       /* non existent domain */
#define NOTIMP      4       /* not implemented */
#define REFUSED     5       /* query refused */
    /* non standard */
#define NOCHANGE    0xf     /* update failed to change db */

/*
 * Type values for resources and queries
 */
#define T_A     1       /* host address */
#define T_NS        2       /* authoritative server */
#define T_MD        3       /* mail destination */
#define T_MF        4       /* mail forwarder */
#define T_CNAME     5       /* connonical name */
#define T_SOA       6       /* start of authority zone */
#define T_MB        7       /* mailbox domain name */
#define T_MG        8       /* mail group member */
#define T_MR        9       /* mail rename name */
#define T_NULL      10      /* null resource record */
#define T_WKS       11      /* well known service */
#define T_PTR       12      /* domain name pointer */
#define T_HINFO     13      /* host information */
#define T_MINFO     14      /* mailbox information */
#define T_MX        15      /* mail routing information */
#define T_TXT       16      /* text strings */
#define T_AFSDB     18      /* AFS database servers */
    /* non standard */
#define T_UINFO     100     /* user (finger) information */
#define T_UID       101     /* user ID */
#define T_GID       102     /* group ID */
#define T_UNSPEC    103     /* Unspecified format (binary data) */
    /* Query type values which do not appear in resource records */
#define T_AXFR      252     /* transfer zone of authority */
#define T_MAILB     253     /* transfer mailbox records */
#define T_MAILA     254     /* transfer mail agent records */
#define T_ANY       255     /* wildcard match */

/*
 * Values for class field
 */

#define C_IN        1       /* the arpa internet */
#define C_CHAOS     3       /* for chaos net at MIT */
#define C_HS        4       /* for Hesiod name server at MIT */
    /* Query class values which do not appear in resource records */
#define C_ANY       255     /* wildcard match */

/*
 * Status return codes for T_UNSPEC conversion routines
 */
#define CONV_SUCCESS 0
#define CONV_OVERFLOW -1
#define CONV_BADFMT -2
#define CONV_BADCKSUM -3
#define CONV_BADBUFLEN -4

#ifndef BYTE_ORDER
#define LITTLE_ENDIAN   1234    /* least-significant byte first (vax) */
#define BIG_ENDIAN  4321    /* most-significant byte first (IBM, net) */
#define PDP_ENDIAN  3412    /* LSB first in word, MSW first in long (pdp) */

#if defined(vax) || defined(ns32000) || defined(sun386) || defined(MIPSEL) || \
    defined(BIT_ZERO_ON_RIGHT)
#define BYTE_ORDER  LITTLE_ENDIAN

#endif
#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
    defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
    defined(MIPSEB) || defined(ux10) || defined (BIT_ZERO_ON_LEFT)
#define BYTE_ORDER  BIG_ENDIAN
#endif
#ifndef BYTE_ORDER      /* still not defined */
#if defined(u3b2) || defined(m68k)
#define BYTE_ORDER  BIG_ENDIAN
#endif
#if defined(i286) || defined(i386) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) || \
      defined(IA64)
#define BYTE_ORDER  LITTLE_ENDIAN
#endif
#endif /* ~BYTE_ORDER */
#endif /* BYTE_ORDER */

#ifndef BYTE_ORDER
    /* you must determine what the correct bit order is for your compiler */
    UNDEFINED_BIT_ORDER;
#endif
/*
 * Structure for query header, the order of the fields is machine and
 * compiler dependent, in our case, the bits within a byte are assignd
 * least significant first, while the order of transmition is most
 * significant first.  This requires a somewhat confusing rearrangement.
 */

typedef struct {
    unsigned short  id;     /* query identification number */
#if BYTE_ORDER == BIG_ENDIAN
            /* fields in third byte */
    unsigned char   qr:1;       /* response flag */
    unsigned char   opcode:4;   /* purpose of message */
    unsigned char   aa:1;       /* authoritive answer */
    unsigned char   tc:1;       /* truncated message */
    unsigned char   rd:1;       /* recursion desired */
            /* fields in fourth byte */
    unsigned char   ra:1;       /* recursion available */
    unsigned char   pr:1;       /* primary server required (non standard) */
    unsigned char   unused:2;   /* unused bits */
    unsigned char   rcode:4;    /* response code */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
            /* fields in third byte */
    unsigned char   rd:1;       /* recursion desired */
    unsigned char   tc:1;       /* truncated message */
    unsigned char   aa:1;       /* authoritive answer */
    unsigned char   opcode:4;   /* purpose of message */
    unsigned char   qr:1;       /* response flag */
            /* fields in fourth byte */
    unsigned char   rcode:4;    /* response code */
    unsigned char   unused:2;   /* unused bits */
    unsigned char   pr:1;       /* primary server required (non standard) */
    unsigned char   ra:1;       /* recursion available */
#endif
            /* remaining bytes */
    unsigned short  qdcount;    /* number of question entries */
    unsigned short  ancount;    /* number of answer entries */
    unsigned short  nscount;    /* number of authority entries */
    unsigned short  arcount;    /* number of resource entries */
} HEADER;

/*
 * Defines for handling compressed domain names
 */
#define INDIR_MASK  0xc0

/*
 * Structure for passing resource records around.
 */
struct rrec {
    short           r_zone;         /* zone number */
    short           r_class;        /* class number */
    short           r_type;         /* type number */
    unsigned long   r_ttl;          /* time to live */
    int         r_size;         /* size of data area */
    char           *r_data;             /* pointer to data */
};

extern  unsigned short  _getshort(char *);
extern  unsigned long   _getlong(char *);

/*
 * Inline versions of get/put short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be unsigned char *.
 */
#define GETSHORT(s, cp) { \
    (s) = *(cp)++ << 8; \
    (s) |= *(cp)++; \
}

#define GETLONG(l, cp) { \
    (l) = *(cp)++ << 8; \
    (l) |= *(cp)++; (l) <<= 8; \
    (l) |= *(cp)++; (l) <<= 8; \
    (l) |= *(cp)++; \
}


#define PUTSHORT(s, cp) { \
    *(cp)++ = (s) >> 8; \
    *(cp)++ = (s); \
}

/*
 * Warning: PUTLONG destroys its first argument.
 */
#define PUTLONG(l, cp) { \
    (cp)[3] = l; \
    (cp)[2] = (l >>= 8); \
    (cp)[1] = (l >>= 8); \
    (cp)[0] = l >> 8; \
    (cp) += sizeof(unsigned long); \
}

#endif  // _NAMESER_INCLUDED
