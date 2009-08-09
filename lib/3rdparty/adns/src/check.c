/*
 * check.c
 * - consistency checks
 */
/*
 *  This file is
 *    Copyright (C) 1997-1999 Ian Jackson <ian@davenant.greenend.org.uk>
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

#include "internal.h"

void adns_checkconsistency(adns_state ads, adns_query qu) {
  adns__consistency(ads,qu,cc_user);
}

/* The original macro. Too gnuish for other compilers */
#if 0
#define DLIST_CHECK(list, nodevar, part, body)					\
  if ((list).head) {								\
    assert(! (list).head->part back);						\
    for ((nodevar)= (list).head; (nodevar); (nodevar)= (nodevar)->part next) {	\
      assert((nodevar)->part next						\
	     ? (nodevar) == (nodevar)->part next->part back			\
	     : (nodevar) == (list).tail);					\
      body									\
    }										\
  }
#endif /* 0 */

#define DLIST_CHECK1(list, nodevar, body)					\
  if ((list).head) {								\
    assert(! (list).head->back);						\
    for ((nodevar)= (list).head; (nodevar); (nodevar)= (nodevar)->next) {	\
      assert((nodevar)->next						\
	     ? (nodevar) == (nodevar)->next->back			\
	     : (nodevar) == (list).tail);					\
      body									\
    }										\
  }

#define DLIST_CHECK2(list, nodevar, part, body)					\
  if ((list).head) {								\
    assert(! (list).head->part.back);						\
    for ((nodevar)= (list).head; (nodevar); (nodevar)= (nodevar)->part.next) {	\
      assert((nodevar)->part.next						\
	     ? (nodevar) == (nodevar)->part.next->part.back			\
	     : (nodevar) == (list).tail);					\
      body									\
    }										\
  }

#define DLIST_ASSERTON(node, nodevar, list, part)				\
  do {										\
    for ((nodevar)= (list).head;						\
	 (nodevar) != (node);							\
	 (nodevar)= (nodevar)->part next) {					\
      assert((nodevar));							\
    }										\
  } while(0)

static void checkc_query_alloc(adns_state ads, adns_query qu) {
  allocnode *an;


  DLIST_CHECK1(qu->allocations, an, {
  });
}

static void checkc_query(adns_state ads, adns_query qu) {
  adns_query child;

  assert(qu->udpnextserver < ads->nservers);
  assert(!(qu->udpsent & (~0UL << ads->nservers)));
  assert(qu->search_pos <= ads->nsearchlist);
  if (qu->parent) DLIST_ASSERTON(qu, child, qu->parent->children, siblings.);
}

static void checkc_notcpbuf(adns_state ads) {
  assert(!ads->tcpsend.used);
  assert(!ads->tcprecv.used);
  assert(!ads->tcprecv_skip);
}

static void checkc_global(adns_state ads) {
  int i;
  
  assert(ads->udpsocket >= 0);

  for (i=0; i<ads->nsortlist; i++)
    assert(!(ads->sortlist[i].base.s_addr & ~ads->sortlist[i].mask.s_addr));

  assert(ads->tcpserver >= 0 && ads->tcpserver < ads->nservers);

  switch (ads->tcpstate) {
  case server_connecting:
    assert(ads->tcpsocket >= 0);
    checkc_notcpbuf(ads);
    break;
  case server_disconnected:
  case server_broken:
    assert(ads->tcpsocket == -1);
    checkc_notcpbuf(ads);
    break;
  case server_ok:
    assert(ads->tcpsocket >= 0);
    assert(ads->tcprecv_skip <= ads->tcprecv.used);
    break;
  default:
    assert(!"ads->tcpstate value");
  }

  assert(ads->searchlist || !ads->nsearchlist);
}

static void checkc_queue_udpw(adns_state ads) {
  adns_query qu;

  DLIST_CHECK1(ads->udpw, qu, {
    assert(qu->state==query_tosend);
    assert(qu->retries <= UDPMAXRETRIES);
    assert(qu->udpsent);
    assert(!qu->children.head && !qu->children.tail);
    checkc_query(ads,qu);
    checkc_query_alloc(ads,qu);
  });
}

static void checkc_queue_tcpw(adns_state ads) {
  adns_query qu;

  DLIST_CHECK1(ads->tcpw, qu, {
    assert(qu->state==query_tcpw);
    assert(!qu->children.head && !qu->children.tail);
    assert(qu->retries <= ads->nservers+1);
    checkc_query(ads,qu);
    checkc_query_alloc(ads,qu);
  });
}

static void checkc_queue_childw(adns_state ads) {
  adns_query parent, child;

  DLIST_CHECK1(ads->childw, parent, {
    assert(parent->state == query_childw);
    assert(parent->children.head);
    DLIST_CHECK2(parent->children, child, siblings, {
      assert(child->parent == parent);
      assert(child->state != query_done);
    });
    checkc_query(ads,parent);
    checkc_query_alloc(ads,parent);
  });
}

static void checkc_queue_output(adns_state ads) {
  adns_query qu;

  DLIST_CHECK1(ads->output, qu, {
    assert(qu->state == query_done);
    assert(!qu->children.head && !qu->children.tail);
    assert(!qu->parent);
    assert(!qu->allocations.head && !qu->allocations.tail);
    checkc_query(ads,qu);
  });
}

void adns__consistency(adns_state ads, adns_query qu, consistency_checks cc) {
  adns_query search;

  switch (cc) {
  case cc_user:
    break;
  case cc_entex:
    if (!(ads->iflags & adns_if_checkc_entex)) return;
    break;
  case cc_freq:
    if ((ads->iflags & adns_if_checkc_freq) != adns_if_checkc_freq) return;
    break;
  default:
    abort();
  }

  checkc_global(ads);
  checkc_queue_udpw(ads);
  checkc_queue_tcpw(ads);
  checkc_queue_childw(ads);
  checkc_queue_output(ads);

  if (qu) {
    switch (qu->state) {
    case query_tosend:
      DLIST_ASSERTON(qu, search, ads->udpw, );
      break;
    case query_tcpw:
      DLIST_ASSERTON(qu, search, ads->tcpw, );
      break;
    case query_childw:
      DLIST_ASSERTON(qu, search, ads->childw, );
      break;
    case query_done:
      DLIST_ASSERTON(qu, search, ads->output, );
      break;
    default:
      assert(!"specific query state");
    }
  }
}
