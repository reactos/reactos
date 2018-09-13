/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  RESOLV.H
 *
 ******************************************************************/

/*
 *   /usr/projects/tcp/SCCS.rel3/rel/src/include/0/s.resolv.h
 *  @(#)resolv.h    5.3
 *
 *  Last delta created  14:05:35 3/4/91
 *  This file extracted 11:19:25 3/8/91
 *
 *  Modifications:
 *
 *      GSS 20 Jul 90   New File
 */

/*
 * Copyright (c) 1983, 1987, 1989 The Regents of the University of California.
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
 *  @(#)resolv.h    5.10 (Berkeley) 6/1/90
 */

#ifndef _RESOLV_INCLUDED
#define _RESOLV_INCLUDED

/*
 * Global defines and variables for resolver stub.
 */
#define MAXNS       3       /* max # name servers we'll track */
#define MAXDFLSRCH  3       /* # default domain levels to try */
#define MAXDNSRCH   6       /* max # domains in search path */
#define LOCALDOMAINPARTS 2      /* min levels in name that is "local" */

#define RES_TIMEOUT 4       /* min. seconds between retries */

struct state {
    int  retrans;          /* retransmition time interval */
    int  retry;            /* number of times to retransmit */
    long options;          /* option flags - see below. */
    int  nscount;          /* number of name servers */
    struct   sockaddr_in nsaddr_list[MAXNS];  /* address of name server */
#define nsaddr   nsaddr_list[0]        /* for backward compatibility */
    unsigned short  id;        /* current packet id */
    char     defdname[MAXDNAME];       /* default domain */
    char    *dnsrch[MAXDNSRCH+1];      /* components of domain to search */
};

/*
 * Resolver options
 */
#define RES_INIT    0x0001      /* address initialized */
#define RES_DEBUG   0x0002      /* print debug messages */
#define RES_AAONLY  0x0004      /* authoritative answers only */
#define RES_USEVC   0x0008      /* use virtual circuit */
#define RES_PRIMARY 0x0010      /* query primary server only */
#define RES_IGNTC   0x0020      /* ignore trucation errors */
#define RES_RECURSE 0x0040      /* recursion desired */
#define RES_DEFNAMES    0x0080      /* use default domain name */
#define RES_STAYOPEN    0x0100      /* Keep TCP socket open */
#define RES_DNSRCH  0x0200      /* search up local domain tree */
#define RES_MODE_HOST_ONLY 0x0400          /* use the host file only */
#define RES_MODE_DNS_ONLY  0x0800          /* use the DNS only */
#define RES_MODE_HOST_DNS  0x1000          /* use the host file then the DNS */
#define RES_MODE_DNS_HOST  0x2000          /* use the DNS then the host file */

#define RES_DEFAULT (RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

extern struct state _res;
extern char *p_cdname(), *p_rr(), *p_type(), *p_class(), *p_time();


//
// Resolver function prototypes
//

int
dn_expand(
    IN  unsigned char *msg,
    IN  unsigned char *eomorig,
    IN  unsigned char *comp_dn,
    OUT unsigned char *exp_dn,
    IN  int            length
    );

int
dn_comp(
    IN      unsigned char  *exp_dn,
    OUT     unsigned char  *comp_dn,
    IN      int             length,
    IN      unsigned char **dnptrs,     OPTIONAL
    IN OUT  unsigned char **lastdnptr   OPTIONAL
    );

int
res_init(
    void
    );

int
res_send(
    IN  char *buf,
    IN  int buflen,
    OUT char *answer,
    IN  int anslen
    );

int
res_query(
    IN  char          *name,      /* domain name */
    IN  int            Class,     /* class of query */
    IN  int            type,      /* type of query */
    OUT unsigned char *answer,    /* buffer to put answer */
    IN  int            anslen     /* size of answer buffer */
    );

int
res_search(
    IN  char           *name,     /* domain name */
    IN  int            Class,     /* class of query */
    IN  int            type,      /* type of query */
    OUT unsigned char *answer,    /* buffer to put answer */
    IN  int            anslen     /* size of answer */
    );

int
res_mkquery(
    IN  int          op,             // opcode of query
    IN  char        *dname,          // domain name
    IN  int          Class,                  // class of query
    IN  int          type,               // type of query
    IN  char        *data,    OPTIONAL       // resource record data
    IN  int          datalen, OPTIONAL       // length of data
    IN  struct rrec *newrr,   OPTIONAL       // new rr for modify or append
    OUT char        *buf,            // buffer to put query
    IN  int          buflen                  // size of buffer
    );

#endif    // _RESOLV_INCLUDED
