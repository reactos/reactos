/*
 * internal.h
 * - declarations of private objects with external linkage (adns__*)
 * - definitons of internal macros
 * - comments regarding library data structures
 */
/*
 *  This file is
 *    Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>
 *
 *  It is part of adns, which is
 *    Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>
 *    Copyright (C) 1999-2000 Tony Finch <dot@dotat.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef ADNS_INTERNAL_H_INCLUDED
#define ADNS_INTERNAL_H_INCLUDED

#include "config.h"

#ifndef ADNS_JGAA_WIN32
typedef unsigned char byte;
#endif


#include <stdarg.h>
#include <assert.h>

#ifndef ADNS_JGAA_WIN32
# include <unistd.h>
#endif
#include <signal.h>
#include <errno.h>
#include <string.h>

#ifndef ADNS_JGAA_WIN32
#  include <sys/time.h>
#endif

#include "adns.h"
#include "dlist.h"

#ifdef ADNS_REGRESS_TEST
# include "hredirect.h"
#endif

/* Configuration and constants */

#define MAXSERVERS 5
#define MAXSORTLIST 15
#define UDPMAXRETRIES 15
#define UDPRETRYMS 2000
#define TCPWAITMS 30000
#define TCPCONNMS 14000
#define TCPIDLEMS 30000
#define MAXTTLBELIEVE (7*86400) /* any TTL > 7 days is capped */

#define DNS_PORT 53
#define DNS_MAXUDP 512
#define DNS_MAXLABEL 63
#define DNS_MAXDOMAIN 255
#define DNS_HDRSIZE 12
#define DNS_IDOFFSET 0
#define DNS_CLASS_IN 1

#define DNS_INADDR_ARPA "in-addr", "arpa"

#define MAX_POLLFDS  ADNS_POLLFDS_RECOMMENDED

typedef enum {
  cc_user,
  cc_entex,
  cc_freq
} consistency_checks;

typedef enum {
  rcode_noerror,
  rcode_formaterror,
  rcode_servfail,
  rcode_nxdomain,
  rcode_notimp,
  rcode_refused
} dns_rcode;

/* Shared data structures */

typedef union {
  adns_status status;
  char *cp;
  adns_rrtype type;
  int i;
  struct in_addr ia;
  unsigned long ul;
} rr_align;

typedef struct {
  int used, avail;
  byte *buf;
} vbuf;

typedef struct {
  adns_state ads;
  adns_query qu;
  int serv;
  const byte *dgram;
  int dglen, nsstart, nscount, arcount;
  struct timeval now;
} parseinfo;

typedef struct {
  adns_rrtype type;
  const char *rrtname;
  const char *fmtname;
  int rrsz;

  void (*makefinal)(adns_query qu, void *data);
  /* Change memory management of *data.
   * Previously, used alloc_interim, now use alloc_final.
   */

  adns_status (*convstring)(vbuf *vb, const void *data);
  /* Converts the RR data to a string representation in vbuf.
   * vbuf will be appended to (it must have been initialised),
   * and will not be null-terminated by convstring.
   */

  adns_status (*parse)(const parseinfo *pai, int cbyte, int max, void *store_r);
  /* Parse one RR, in dgram of length dglen, starting at cbyte and
   * extending until at most max.
   *
   * The RR should be stored at *store_r, of length qu->typei->rrsz.
   *
   * If there is an overrun which might indicate truncation, it should set
   * *rdstart to -1; otherwise it may set it to anything else positive.
   *
   * nsstart is the offset of the authority section.
   */

  int (*diff_needswap)(adns_state ads, const void *datap_a, const void *datap_b);
  /* Returns !0 if RR a should be strictly after RR b in the sort order,
   * 0 otherwise.  Must not fail.
   */
} typeinfo;

typedef struct allocnode {
  struct allocnode *next, *back;
} allocnode;

union maxalign {
  byte d[1];
  struct in_addr ia;
  long l;
  void *p;
  void (*fp)(void);
  union maxalign *up;
} data;

typedef struct {
  void *ext;
  void (*callback)(adns_query parent, adns_query child);
  union {
    adns_rr_addr ptr_parent_addr;
    adns_rr_hostaddr *hostaddr;
  } info;
} qcontext;

struct adns__query {
  adns_state ads;
  enum { query_tosend, query_tcpw, query_childw, query_done } state;
  adns_query back, next, parent;
  struct { adns_query head, tail; } children;
  struct { adns_query back, next; } siblings;
  struct { allocnode *head, *tail; } allocations;
  int interim_allocd, preserved_allocd;
  void *final_allocspace;

  const typeinfo *typei;
  byte *query_dgram;
  int query_dglen;

  vbuf vb;
  /* General-purpose messing-about buffer.
   * Wherever a `big' interface is crossed, this may be corrupted/changed
   * unless otherwise specified.
   */

  adns_answer *answer;
  /* This is allocated when a query is submitted, to avoid being unable
   * to relate errors to queries if we run out of memory.  During
   * query processing status, rrs is 0.  cname is set if
   * we found a cname (this corresponds to cname_dgram in the query
   * structure).  type is set from the word go.  nrrs and rrs
   * are set together, when we find how many rrs there are.
   * owner is set during querying unless we're doing searchlist,
   * in which case it is set only when we find an answer.
   */

  byte *cname_dgram;
  int cname_dglen, cname_begin;
  /* If non-0, has been allocated using . */

  vbuf search_vb;
  int search_origlen, search_pos, search_doneabs;
  /* Used by the searching algorithm.  The query domain in textual form
   * is copied into the vbuf, and _origlen set to its length.  Then
   * we walk the searchlist, if we want to.  _pos says where we are
   * (next entry to try), and _doneabs says whether we've done the
   * absolute query yet (0=not yet, 1=done, -1=must do straight away,
   * but not done yet).  If flags doesn't have adns_qf_search then
   * the vbuf is initialised but empty and everything else is zero.
   */

  int id, flags, retries;
  int udpnextserver;
  unsigned long udpsent; /* bitmap indexed by server */
  struct timeval timeout;
  time_t expires; /* Earliest expiry time of any record we used. */

  qcontext ctx;

  /* Possible states:
   *
   *  state   Queue   child  id   nextudpserver  udpsent     tcpfailed
   *
   *  tosend  NONE    null   >=0  0              zero        zero
   *  tosend  udpw    null   >=0  any            nonzero     zero
   *  tosend  NONE    null   >=0  any            nonzero     zero
   *
   *  tcpw    tcpw    null   >=0  irrelevant     any         any
   *
   *  child   childw  set    >=0  irrelevant     irrelevant  irrelevant
   *  child   NONE    null   >=0  irrelevant     irrelevant  irrelevant
   *  done    output  null   -1   irrelevant     irrelevant  irrelevant
   *
   * Queries are only not on a queue when they are actually being processed.
   * Queries in state tcpw/tcpw have been sent (or are in the to-send buffer)
   * iff the tcp connection is in state server_ok.
   *
   *			      +------------------------+
   *             START -----> |      tosend/NONE       |
   *			      +------------------------+
   *                         /                       |\  \
   *        too big for UDP /             UDP timeout  \  \ send via UDP
   *        send via TCP   /              more retries  \  \
   *        when conn'd   /                  desired     \  \
   *                     |     	       	       	       	  |  |
   *                     v				  |  v
   *              +-----------+         	    	+-------------+
   *              | tcpw/tcpw | ________                | tosend/udpw |
   *              +-----------+         \	    	+-------------+
   *                 |    |              |     UDP timeout | |
   *                 |    |              |      no more    | |
   *                 |    |              |      retries    | |
   *                  \   | TCP died     |      desired    | |
   *                   \   \ no more     |                 | |
   *                    \   \ servers    | TCP            /  |
   *                     \   \ to try    | timeout       /   |
   *                  got \   \          v             |_    | got
   *                 reply \   _| +------------------+      / reply
   *   	       	       	    \  	  | done/output FAIL |     /
   *                         \    +------------------+    /
   *                          \                          /
   *                           _|                      |_
   *                             (..... got reply ....)
   *                              /                   \
   *        need child query/ies /                     \ no child query
   *                            /                       \
   *                          |_                         _|
   *		   +---------------+		       +----------------+
   *               | childw/childw | ----------------> | done/output OK |
   *               +---------------+  children done    +----------------+
   */
};

struct query_queue { adns_query head, tail; };

struct adns__state {
  adns_initflags iflags;
  FILE *diagfile;
  int configerrno;
  struct query_queue udpw, tcpw, childw, output;
  adns_query forallnext;
  int nextid;
  ADNS_SOCKET udpsocket, tcpsocket;
  vbuf tcpsend, tcprecv;
  int nservers, nsortlist, nsearchlist, searchndots, tcpserver, tcprecv_skip;
  enum adns__tcpstate {
    server_disconnected, server_connecting,
    server_ok, server_broken
  } tcpstate;
  struct timeval tcptimeout;
  /* This will have tv_sec==0 if it is not valid.  It will always be
   * valid if tcpstate _connecting.  When _ok, it will be nonzero if
   * we are idle (ie, tcpw queue is empty), in which case it is the
   * absolute time when we will close the connection.
   */
#ifndef ADNS_JGAA_WIN32
  struct sigaction stdsigpipe;
  sigset_t stdsigmask;
#endif
  struct pollfd pollfds_buf[MAX_POLLFDS];
  struct server {
    struct in_addr addr;
  } servers[MAXSERVERS];
  struct sortlist {
    struct in_addr base, mask;
  } sortlist[MAXSORTLIST];
  char **searchlist;
};

/* From setup.c: */

int adns__setnonblock(adns_state ads, ADNS_SOCKET fd); /* => errno value */

/* From general.c: */

void adns__vdiag(adns_state ads, const char *pfx, adns_initflags prevent,
		 int serv, adns_query qu, const char *fmt, va_list al);

void adns__debug(adns_state ads, int serv, adns_query qu,
		 const char *fmt, ...) PRINTFFORMAT(4,5);
void adns__warn(adns_state ads, int serv, adns_query qu,
		const char *fmt, ...) PRINTFFORMAT(4,5);
void adns__diag(adns_state ads, int serv, adns_query qu,
		const char *fmt, ...) PRINTFFORMAT(4,5);

int adns__vbuf_ensure(vbuf *vb, int want);
int adns__vbuf_appendstr(vbuf *vb, const char *data); /* does not include nul */
int adns__vbuf_append(vbuf *vb, const byte *data, int len);
/* 1=>success, 0=>realloc failed */
void adns__vbuf_appendq(vbuf *vb, const byte *data, int len);
void adns__vbuf_init(vbuf *vb);
void adns__vbuf_free(vbuf *vb);

const char *adns__diag_domain(adns_state ads, int serv, adns_query qu,
			      vbuf *vb, const byte *dgram, int dglen, int cbyte);
/* Unpicks a domain in a datagram and returns a string suitable for
 * printing it as.  Never fails - if an error occurs, it will
 * return some kind of string describing the error.
 *
 * serv may be -1 and qu may be 0.  vb must have been initialised,
 * and will be left in an arbitrary consistent state.
 *
 * Returns either vb->buf, or a pointer to a string literal.  Do not modify
 * vb before using the return value.
 */

void adns__isort(void *array, int nobjs, int sz, void *tempbuf,
		 int (*needswap)(void *context, const void *a, const void *b),
		 void *context);
/* Does an insertion sort of array which must contain nobjs objects
 * each sz bytes long.  tempbuf must point to a buffer at least
 * sz bytes long.  needswap should return !0 if a>b (strictly, ie
 * wrong order) 0 if a<=b (ie, order is fine).
 */

void adns__sigpipe_protect(adns_state);
void adns__sigpipe_unprotect(adns_state);
/* If SIGPIPE protection is not disabled, will block all signals except
 * SIGPIPE, and set SIGPIPE's disposition to SIG_IGN.  (And then restore.)
 * Each call to _protect must be followed by a call to _unprotect before
 * any significant amount of code gets to run, since the old signal mask
 * is stored in the adns structure.
 */

/* From transmit.c: */

adns_status adns__mkquery(adns_state ads, vbuf *vb, int *id_r,
			  const char *owner, int ol,
			  const typeinfo *typei, adns_queryflags flags);
/* Assembles a query packet in vb.  A new id is allocated and returned.
 */

adns_status adns__mkquery_frdgram(adns_state ads, vbuf *vb, int *id_r,
				  const byte *qd_dgram, int qd_dglen, int qd_begin,
				  adns_rrtype type, adns_queryflags flags);
/* Same as adns__mkquery, but takes the owner domain from an existing datagram.
 * That domain must be correct and untruncated.
 */

void adns__querysend_tcp(adns_query qu, struct timeval now);
/* Query must be in state tcpw/tcpw; it will be sent if possible and
 * no further processing can be done on it for now.  The connection
 * might be broken, but no reconnect will be attempted.
 */

void adns__query_send(adns_query qu, struct timeval now);
/* Query must be in state tosend/NONE; it will be moved to a new state,
 * and no further processing can be done on it for now.
 * (Resulting state is one of udp/timew, tcpwait/timew (if server not connected),
 *  tcpsent/timew, child/childw or done/output.)
 * __query_send may decide to use either UDP or TCP depending whether
 * _qf_usevc is set (or has become set) and whether the query is too
 * large.
 */

/* From query.c: */

adns_status adns__internal_submit(adns_state ads, adns_query *query_r,
				  const typeinfo *typei, vbuf *qumsg_vb, int id,
				  adns_queryflags flags, struct timeval now,
				  const qcontext *ctx);
/* Submits a query (for internal use, called during external submits).
 *
 * The new query is returned in *query_r, or we return adns_s_nomemory.
 *
 * The query datagram should already have been assembled in qumsg_vb;
 * the memory for it is _taken over_ by this routine whether it
 * succeeds or fails (if it succeeds, the vbuf is reused for qu->vb).
 *
 * *ctx is copied byte-for-byte into the query.
 *
 * When the child query is done, ctx->callback will be called.  The
 * child will already have been taken off both the global list of
 * queries in ads and the list of children in the parent.  The child
 * will be freed when the callback returns.  The parent will have been
 * taken off the global childw queue.
 *
 * The callback should either call adns__query_done, if it is
 * complete, or adns__query_fail, if an error has occurred, in which
 * case the other children (if any) will be cancelled.  If the parent
 * has more unfinished children (or has just submitted more) then the
 * callback may choose to wait for them - it must then put the parent
 * back on the childw queue.
 */

void adns__search_next(adns_state ads, adns_query qu, struct timeval now);
/* Walks down the searchlist for a query with adns_qf_search.
 * The query should have just had a negative response, or not had
 * any queries sent yet, and should not be on any queue.
 * The query_dgram if any will be freed and forgotten and a new
 * one constructed from the search_* members of the query.
 *
 * Cannot fail (in case of error, calls adns__query_fail).
 */

void *adns__alloc_interim(adns_query qu, size_t sz);
void *adns__alloc_preserved(adns_query qu, size_t sz);
/* Allocates some memory, and records which query it came from
 * and how much there was.
 *
 * If an error occurs in the query, all the memory from _interim is
 * simply freed.  If the query succeeds, one large buffer will be made
 * which is big enough for all these allocations, and then
 * adns__alloc_final will get memory from this buffer.
 *
 * _alloc_interim can fail (and return 0).
 * The caller must ensure that the query is failed.
 *
 * The memory from _preserved is is kept and transferred into the
 * larger buffer - unless we run out of memory, in which case it too
 * is freed.  When you use _preserved you have to add code to the
 * x_nomem error exit case in adns__makefinal_query to clear out the
 * pointers you made to those allocations, because that's when they're
 * thrown away; you should also make a note in the declaration of
 * those pointer variables, to note that they are _preserved rather
 * than _interim.  If they're in the answer, note it here:
 *  answer->cname and answer->owner are _preserved.
 */

void adns__transfer_interim(adns_query from, adns_query to, void *block, size_t sz);
/* Transfers an interim allocation from one query to another, so that
 * the `to' query will have room for the data when we get to makefinal
 * and so that the free will happen when the `to' query is freed
 * rather than the `from' query.
 *
 * It is legal to call adns__transfer_interim with a null pointer; this
 * has no effect.
 *
 * _transfer_interim also ensures that the expiry time of the `to' query
 * is no later than that of the `from' query, so that child queries'
 * TTLs get inherited by their parents.
 */

void *adns__alloc_mine(adns_query qu, size_t sz);
/* Like _interim, but does not record the length for later
 * copying into the answer.  This just ensures that the memory
 * will be freed when we're done with the query.
 */

void *adns__alloc_final(adns_query qu, size_t sz);
/* Cannot fail, and cannot return 0.
 */

void adns__makefinal_block(adns_query qu, void **blpp, size_t sz);
void adns__makefinal_str(adns_query qu, char **strp);

void adns__reset_preserved(adns_query qu);
/* Resets all of the memory management stuff etc. to take account of
 * only the _preserved stuff from _alloc_preserved.  Used when we find
 * an error somewhere and want to just report the error (with perhaps
 * CNAME, owner, etc. info), and also when we're halfway through RRs
 * in a datagram and discover that we need to retry the query.
 */

void adns__query_done(adns_query qu);
void adns__query_fail(adns_query qu, adns_status stat);

/* From reply.c: */

void adns__procdgram(adns_state ads, const byte *dgram, int len,
		     int serv, int viatcp, struct timeval now);
/* This function is allowed to cause new datagrams to be constructed
 * and sent, or even new queries to be started.  However,
 * query-sending functions are not allowed to call any general event
 * loop functions in case they accidentally call this.
 *
 * Ie, receiving functions may call sending functions.
 * Sending functions may NOT call receiving functions.
 */

/* From types.c: */

const typeinfo *adns__findtype(adns_rrtype type);

/* From parse.c: */

typedef struct {
  adns_state ads;
  adns_query qu;
  int serv;
  const byte *dgram;
  int dglen, max, cbyte, namelen;
  int *dmend_r;
} findlabel_state;

void adns__findlabel_start(findlabel_state *fls, adns_state ads,
			   int serv, adns_query qu,
			   const byte *dgram, int dglen, int max,
			   int dmbegin, int *dmend_rlater);
/* Finds labels in a domain in a datagram.
 *
 * Call this routine first.
 * dmend_rlater may be null.  ads (and of course fls) may not be.
 * serv may be -1, qu may be null - they are for error reporting.
 */

adns_status adns__findlabel_next(findlabel_state *fls, int *lablen_r, int *labstart_r);
/* Then, call this one repeatedly.
 *
 * It will return adns_s_ok if all is well, and tell you the length
 * and start of successive labels.  labstart_r may be null, but
 * lablen_r must not be.
 *
 * After the last label, it will return with *lablen_r zero.
 * Do not then call it again; instead, just throw away the findlabel_state.
 *
 * *dmend_rlater will have been set to point to the next part of
 * the datagram after the label (or after the uncompressed part,
 * if compression was used).  *namelen_rlater will have been set
 * to the length of the domain name (total length of labels plus
 * 1 for each intervening dot).
 *
 * If the datagram appears to be truncated, *lablen_r will be -1.
 * *dmend_rlater, *labstart_r and *namelen_r may contain garbage.
 * Do not call _next again.
 *
 * There may also be errors, in which case *dmend_rlater,
 * *namelen_rlater, *lablen_r and *labstart_r may contain garbage.
 * Do not then call findlabel_next again.
 */

typedef enum {
  pdf_quoteok= 0x001
} parsedomain_flags;

adns_status adns__parse_domain(adns_state ads, int serv, adns_query qu,
			       vbuf *vb, adns_queryflags flags,
			       const byte *dgram, int dglen, int *cbyte_io, int max);
/* vb must already have been initialised; it will be reset if necessary.
 * If there is truncation, vb->used will be set to 0; otherwise
 * (if there is no error) vb will be null-terminated.
 * If there is an error vb and *cbyte_io may be left indeterminate.
 *
 * serv may be -1 and qu may be 0 - they are used for error reporting only.
 */

adns_status adns__parse_domain_more(findlabel_state *fls, adns_state ads,
				    adns_query qu, vbuf *vb, parsedomain_flags flags,
				    const byte *dgram);
/* Like adns__parse_domain, but you pass it a pre-initialised findlabel_state,
 * for continuing an existing domain or some such of some kind.  Also, unlike
 * _parse_domain, the domain data will be appended to vb, rather than replacing
 * the existing contents.
 */

adns_status adns__findrr(adns_query qu, int serv,
			 const byte *dgram, int dglen, int *cbyte_io,
			 int *type_r, int *class_r, unsigned long *ttl_r,
			 int *rdlen_r, int *rdstart_r,
			 int *ownermatchedquery_r);
/* Finds the extent and some of the contents of an RR in a datagram
 * and does some checks.  The datagram is *dgram, length dglen, and
 * the RR starts at *cbyte_io (which is updated afterwards to point
 * to the end of the RR).
 *
 * The type, class, TTL and RRdata length and start are returned iff
 * the corresponding pointer variables are not null.  type_r, class_r
 * and ttl_r may not be null.  The TTL will be capped.
 *
 * If ownermatchedquery_r != 0 then the owner domain of this
 * RR will be compared with that in the query (or, if the query
 * has gone to a CNAME lookup, with the canonical name).
 * In this case, *ownermatchedquery_r will be set to 0 or 1.
 * The query datagram (or CNAME datagram) MUST be valid and not truncated.
 *
 * If there is truncation then *type_r will be set to -1 and
 * *cbyte_io, *class_r, *rdlen_r, *rdstart_r and *eo_matched_r will be
 * undefined.
 *
 * qu must obviously be non-null.
 *
 * If an error is returned then *type_r will be undefined too.
 */

adns_status adns__findrr_anychk(adns_query qu, int serv,
				const byte *dgram, int dglen, int *cbyte_io,
				int *type_r, int *class_r, unsigned long *ttl_r,
				int *rdlen_r, int *rdstart_r,
				const byte *eo_dgram, int eo_dglen, int eo_cbyte,
				int *eo_matched_r);
/* Like adns__findrr_checked, except that the datagram and
 * owner to compare with can be specified explicitly.
 *
 * If the caller thinks they know what the owner of the RR ought to
 * be they can pass in details in eo_*: this is another (or perhaps
 * the same datagram), and a pointer to where the putative owner
 * starts in that datagram.  In this case *eo_matched_r will be set
 * to 1 if the datagram matched or 0 if it did not.  Either
 * both eo_dgram and eo_matched_r must both be non-null, or they
 * must both be null (in which case eo_dglen and eo_cbyte will be ignored).
 * The eo datagram and contained owner domain MUST be valid and
 * untruncated.
 */

void adns__update_expires(adns_query qu, unsigned long ttl, struct timeval now);
/* Updates the `expires' field in the query, so that it doesn't exceed
 * now + ttl.
 */

int vbuf__append_quoted1035(vbuf *vb, const byte *buf, int len);

/* From event.c: */

void adns__tcp_broken(adns_state ads, const char *what, const char *why);
/* what and why may be both 0, or both non-0. */

void adns__tcp_tryconnect(adns_state ads, struct timeval now);

void adns__autosys(adns_state ads, struct timeval now);
/* Make all the system calls we want to if the application wants us to.
 * Must not be called from within adns internal processing functions,
 * lest we end up in recursive descent !
 */

void adns__must_gettimeofday(adns_state ads, const struct timeval **now_io,
			     struct timeval *tv_buf);

int adns__pollfds(adns_state ads, struct pollfd pollfds_buf[MAX_POLLFDS]);
void adns__fdevents(adns_state ads,
		    const struct pollfd *pollfds, int npollfds,
		    int maxfd, const fd_set *readfds,
		    const fd_set *writefds, const fd_set *exceptfds,
		    struct timeval now, int *r_r);
int adns__internal_check(adns_state ads,
			 adns_query *query_io,
			 adns_answer **answer,
			 void **context_r);

void adns__timeouts(adns_state ads, int act,
		    struct timeval **tv_io, struct timeval *tvbuf,
		    struct timeval now);
/* If act is !0, then this will also deal with the TCP connection
 * if previous events broke it or require it to be connected.
 */

/* From check.c: */

void adns__consistency(adns_state ads, adns_query qu, consistency_checks cc);

/* Useful static inline functions: */

static inline int ctype_whitespace(int c) { return c==' ' || c=='\n' || c=='\t'; }
static inline int ctype_digit(int c) { return c>='0' && c<='9'; }
static inline int ctype_alpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline int ctype_822special(int c) { return strchr("()<>@,;:\\\".[]",c) != 0; }
static inline int ctype_domainunquoted(int c) {
  return ctype_alpha(c) || ctype_digit(c) || (strchr("-_/+",c) != 0);
}

static inline int errno_resources(int e) { return e==ENOMEM || e==ENOBUFS; }

/* Useful macros */

#define MEM_ROUND(sz) \
  (( ((sz)+sizeof(union maxalign)-1) / sizeof(union maxalign) ) \
   * sizeof(union maxalign) )

#define GETIL_B(cb) (((dgram)[(cb)++]) & 0x0ff)
#define GET_B(cb,tv) ((tv)= GETIL_B((cb)))
#define GET_W(cb,tv) ((tv)=0, (tv)|=(GETIL_B((cb))<<8), (tv)|=GETIL_B(cb), (tv))
#define GET_L(cb,tv) ( (tv)=0, \
		       (tv)|=(GETIL_B((cb))<<24), \
		       (tv)|=(GETIL_B((cb))<<16), \
		       (tv)|=(GETIL_B((cb))<<8), \
		       (tv)|=GETIL_B(cb), \
		       (tv) )

#endif
