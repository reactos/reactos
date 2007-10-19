/*
 * reply.c
 * - main handling and parsing routine for received datagrams
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

#ifdef ADNS_JGAA_WIN32
# include "adns_win32.h"
#endif

#include <stdlib.h>

#include "internal.h"

void adns__procdgram(adns_state ads, const byte *dgram, int dglen,
		     int serv, int viatcp, struct timeval now) {
  int cbyte, rrstart, wantedrrs, rri, foundsoa, foundns, cname_here;
  int id, f1, f2, qdcount, ancount, nscount, arcount;
  int flg_ra, flg_rd, flg_tc, flg_qr, opcode;
  int rrtype, rrclass, rdlength, rdstart;
  int anstart, nsstart, arstart;
  int ownermatched, l, nrrs;
  unsigned long ttl, soattl;
  const typeinfo *typei;
  adns_query qu, nqu;
  dns_rcode rcode;
  adns_status st;
  vbuf tempvb;
  byte *newquery, *rrsdata;
  parseinfo pai;

  if (dglen<DNS_HDRSIZE) {
    adns__diag(ads,serv,0,"received datagram too short for message header (%d)",dglen);
    return;
  }
  cbyte= 0;
  GET_W(cbyte,id);
  GET_B(cbyte,f1);
  GET_B(cbyte,f2);
  GET_W(cbyte,qdcount);
  GET_W(cbyte,ancount);
  GET_W(cbyte,nscount);
  GET_W(cbyte,arcount);
  assert(cbyte == DNS_HDRSIZE);

  flg_qr= f1&0x80;
  opcode= (f1&0x78)>>3;
  flg_tc= f1&0x02;
  flg_rd= f1&0x01;
  flg_ra= f2&0x80;
  rcode= (f2&0x0f);

  cname_here= 0;

  if (!flg_qr) {
    adns__diag(ads,serv,0,"server sent us a query, not a response");
    return;
  }
  if (opcode) {
    adns__diag(ads,serv,0,"server sent us unknown opcode %d (wanted 0=QUERY)",opcode);
    return;
  }

  qu= 0;
  /* See if we can find the relevant query, or leave qu=0 otherwise ... */

  if (qdcount == 1) {
    for (qu= viatcp ? ads->tcpw.head : ads->udpw.head; qu; qu= nqu) {
      nqu= qu->next;
      if (qu->id != id) continue;
      if (dglen < qu->query_dglen) continue;
      if (memcmp(qu->query_dgram+DNS_HDRSIZE,
		 dgram+DNS_HDRSIZE,
		 (size_t) qu->query_dglen-DNS_HDRSIZE))
	continue;
      if (viatcp) {
	assert(qu->state == query_tcpw);
      } else {
	assert(qu->state == query_tosend);
	if (!(qu->udpsent & (1<<serv))) continue;
      }
      break;
    }
    if (qu) {
      /* We're definitely going to do something with this query now */
      if (viatcp) LIST_UNLINK(ads->tcpw,qu);
      else LIST_UNLINK(ads->udpw,qu);
    }
  }

  /* If we're going to ignore the packet, we return as soon as we have
   * failed the query (if any) and printed the warning message (if
   * any).
   */
  switch (rcode) {
  case rcode_noerror:
  case rcode_nxdomain:
    break;
  case rcode_formaterror:
    adns__warn(ads,serv,qu,"server cannot understand our query (Format Error)");
    if (qu) adns__query_fail(qu,adns_s_rcodeformaterror);
    return;
  case rcode_servfail:
    if (qu) adns__query_fail(qu,adns_s_rcodeservfail);
    else adns__debug(ads,serv,qu,"server failure on unidentifiable query");
    return;
  case rcode_notimp:
    adns__warn(ads,serv,qu,"server claims not to implement our query");
    if (qu) adns__query_fail(qu,adns_s_rcodenotimplemented);
    return;
  case rcode_refused:
    adns__debug(ads,serv,qu,"server refused our query");
    if (qu) adns__query_fail(qu,adns_s_rcoderefused);
    return;
  default:
    adns__warn(ads,serv,qu,"server gave unknown response code %d",rcode);
    if (qu) adns__query_fail(qu,adns_s_rcodeunknown);
    return;
  }

  if (!qu) {
    if (!qdcount) {
      adns__diag(ads,serv,0,"server sent reply without quoting our question");
    } else if (qdcount>1) {
      adns__diag(ads,serv,0,"server claimed to answer %d questions with one message",
		 qdcount);
    } else if (ads->iflags & adns_if_debug) {
      adns__vbuf_init(&tempvb);
      adns__debug(ads,serv,0,"reply not found, id %02x, query owner %s",
		  id, adns__diag_domain(ads,serv,0,&tempvb,dgram,dglen,DNS_HDRSIZE));
      adns__vbuf_free(&tempvb);
    }
    return;
  }

  /* We're definitely going to do something with this packet and this query now. */

  anstart= qu->query_dglen;
  arstart= -1;

  /* Now, take a look at the answer section, and see if it is complete.
   * If it has any CNAMEs we stuff them in the answer.
   */
  wantedrrs= 0;
  cbyte= anstart;
  for (rri= 0; rri<ancount; rri++) {
    rrstart= cbyte;
    st= adns__findrr(qu,serv, dgram,dglen,&cbyte,
		     &rrtype,&rrclass,&ttl, &rdlength,&rdstart,
		     &ownermatched);
    if (st) { adns__query_fail(qu,st); return; }
    if (rrtype == -1) goto x_truncated;

    if (rrclass != DNS_CLASS_IN) {
      adns__diag(ads,serv,qu,"ignoring answer RR with wrong class %d (expected IN=%d)",
		 rrclass,DNS_CLASS_IN);
      continue;
    }
    if (!ownermatched) {
      if (ads->iflags & adns_if_debug) {
	adns__debug(ads,serv,qu,"ignoring RR with an unexpected owner %s",
		    adns__diag_domain(ads,serv,qu, &qu->vb, dgram,dglen,rrstart));
      }
      continue;
    }
    if (rrtype == adns_r_cname &&
	(qu->typei->type & adns__rrt_typemask) != adns_r_cname) {
      if (qu->flags & adns_qf_cname_forbid) {
	adns__query_fail(qu,adns_s_prohibitedcname);
	return;
      } else if (qu->cname_dgram) { /* Ignore second and subsequent CNAME(s) */
	adns__debug(ads,serv,qu,"allegedly canonical name %s is actually alias for %s",
		    qu->answer->cname,
		    adns__diag_domain(ads,serv,qu, &qu->vb, dgram,dglen,rdstart));
	adns__query_fail(qu,adns_s_prohibitedcname);
	return;
      } else if (wantedrrs) { /* Ignore CNAME(s) after RR(s). */
	adns__debug(ads,serv,qu,"ignoring CNAME (to %s) coexisting with RR",
		    adns__diag_domain(ads,serv,qu, &qu->vb, dgram,dglen,rdstart));
      } else {
	qu->cname_begin= rdstart;
	qu->cname_dglen= dglen;
	st= adns__parse_domain(ads,serv,qu, &qu->vb,
			       qu->flags & adns_qf_quotefail_cname ? 0 : pdf_quoteok,
			       dgram,dglen, &rdstart,rdstart+rdlength);
	if (!qu->vb.used) goto x_truncated;
	if (st) { adns__query_fail(qu,st); return; }
	l= strlen((char*)qu->vb.buf)+1;
	qu->answer->cname= adns__alloc_preserved(qu,(size_t) l);
	if (!qu->answer->cname) { adns__query_fail(qu,adns_s_nomemory); return; }

	qu->cname_dgram= adns__alloc_mine(qu, (size_t) dglen);
	memcpy(qu->cname_dgram,dgram,(size_t) dglen);

	memcpy(qu->answer->cname,qu->vb.buf, (size_t) l);
	cname_here= 1;
	adns__update_expires(qu,ttl,now);
	/* If we find the answer section truncated after this point we restart
	 * the query at the CNAME; if beforehand then we obviously have to use
	 * TCP.  If there is no truncation we can use the whole answer if
	 * it contains the relevant info.
	 */
      }
    } else if (rrtype == ((INT)qu->typei->type & (INT)adns__rrt_typemask)) {
      wantedrrs++;
    } else {
      adns__debug(ads,serv,qu,"ignoring answer RR with irrelevant type %d",rrtype);
    }
  }

  /* We defer handling truncated responses here, in case there was a CNAME
   * which we could use.
   */
  if (flg_tc) goto x_truncated;

  nsstart= cbyte;

  if (!wantedrrs) {
    /* Oops, NODATA or NXDOMAIN or perhaps a referral (which would be a problem) */

    /* RFC2308: NODATA has _either_ a SOA _or_ _no_ NS records in authority section */
    foundsoa= 0; soattl= 0; foundns= 0;
    for (rri= 0; rri<nscount; rri++) {
      rrstart= cbyte;
      st= adns__findrr(qu,serv, dgram,dglen,&cbyte,
		       &rrtype,&rrclass,&ttl, &rdlength,&rdstart, 0);
      if (st) { adns__query_fail(qu,st); return; }
      if (rrtype==-1) goto x_truncated;
      if (rrclass != DNS_CLASS_IN) {
	adns__diag(ads,serv,qu,
		   "ignoring authority RR with wrong class %d (expected IN=%d)",
		   rrclass,DNS_CLASS_IN);
	continue;
      }
      if (rrtype == adns_r_soa_raw) { foundsoa= 1; soattl= ttl; break; }
      else if (rrtype == adns_r_ns_raw) { foundns= 1; }
    }

    if (rcode == rcode_nxdomain) {
      /* We still wanted to look for the SOA so we could find the TTL. */
      adns__update_expires(qu,soattl,now);

      if (qu->flags & adns_qf_search) {
	adns__search_next(ads,qu,now);
      } else {
	adns__query_fail(qu,adns_s_nxdomain);
      }
      return;
    }

    if (foundsoa || !foundns) {
      /* Aha !  A NODATA response, good. */
      adns__update_expires(qu,soattl,now);
      adns__query_fail(qu,adns_s_nodata);
      return;
    }

    /* Now what ?  No relevant answers, no SOA, and at least some NS's.
     * Looks like a referral.  Just one last chance ... if we came across
     * a CNAME in this datagram then we should probably do our own CNAME
     * lookup now in the hope that we won't get a referral again.
     */
    if (cname_here) goto x_restartquery;

    /* Bloody hell, I thought we asked for recursion ? */
    if (!flg_ra) {
      adns__diag(ads,serv,qu,"server is not willing to do recursive lookups for us");
      adns__query_fail(qu,adns_s_norecurse);
    } else {
      if (!flg_rd)
	adns__diag(ads,serv,qu,"server thinks we didn't ask for recursive lookup");
      else
	adns__debug(ads,serv,qu,"server claims to do recursion, but gave us a referral");
      adns__query_fail(qu,adns_s_invalidresponse);
    }
    return;
  }

  /* Now, we have some RRs which we wanted. */

  qu->answer->rrs.untyped= adns__alloc_interim(qu,(size_t) qu->typei->rrsz*wantedrrs);
  if (!qu->answer->rrs.untyped) { adns__query_fail(qu,adns_s_nomemory); return; }

  typei= qu->typei;
  cbyte= anstart;
  rrsdata= qu->answer->rrs.bytes;

  pai.ads= qu->ads;
  pai.qu= qu;
  pai.serv= serv;
  pai.dgram= dgram;
  pai.dglen= dglen;
  pai.nsstart= nsstart;
  pai.nscount= nscount;
  pai.arcount= arcount;
  pai.now= now;

  for (rri=0, nrrs=0; rri<ancount; rri++) {
    st= adns__findrr(qu,serv, dgram,dglen,&cbyte,
		     &rrtype,&rrclass,&ttl, &rdlength,&rdstart,
		     &ownermatched);
    assert(!st); assert(rrtype != -1);
    if (rrclass != DNS_CLASS_IN ||
	rrtype != ((INT)qu->typei->type & (INT)adns__rrt_typemask) ||
	!ownermatched)
      continue;
    adns__update_expires(qu,ttl,now);
    st= typei->parse(&pai, rdstart,rdstart+rdlength, rrsdata+nrrs*typei->rrsz);
    if (st) { adns__query_fail(qu,st); return; }
    if (rdstart==-1) goto x_truncated;
    nrrs++;
  }
  assert(nrrs==wantedrrs);
  qu->answer->nrrs= nrrs;

  /* This may have generated some child queries ... */
  if (qu->children.head) {
    qu->state= query_childw;
    LIST_LINK_TAIL(ads->childw,qu);
    return;
  }
  adns__query_done(qu);
  return;

 x_truncated:

  if (!flg_tc) {
    adns__diag(ads,serv,qu,"server sent datagram which points outside itself");
    adns__query_fail(qu,adns_s_invalidresponse);
    return;
  }
  qu->flags |= adns_qf_usevc;

 x_restartquery:
  if (qu->cname_dgram) {
    st= adns__mkquery_frdgram(qu->ads,&qu->vb,&qu->id,
			      qu->cname_dgram, qu->cname_dglen, qu->cname_begin,
			      qu->typei->type, qu->flags);
    if (st) { adns__query_fail(qu,st); return; }

    newquery= realloc(qu->query_dgram, (size_t) qu->vb.used);
    if (!newquery) { adns__query_fail(qu,adns_s_nomemory); return; }

    qu->query_dgram= newquery;
    qu->query_dglen= qu->vb.used;
    memcpy(newquery,qu->vb.buf, (size_t) qu->vb.used);
  }

  if (qu->state == query_tcpw) qu->state= query_tosend;
  qu->retries= 0;
  adns__reset_preserved(qu);
  adns__query_send(qu,now);
}
