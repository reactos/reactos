/*
 * query.c
 * - overall query management (allocation, completion)
 * - per-query memory management
 * - query submission and cancellation (user-visible and internal)
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
#else
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>
# include <sys/time.h>
#endif

#include "internal.h"

static adns_query query_alloc(adns_state ads, const typeinfo *typei,
			      adns_queryflags flags, struct timeval now) {
  /* Allocate a virgin query and return it. */
  adns_query qu;

  qu= malloc(sizeof(*qu));  if (!qu) return 0;
  qu->answer= malloc(sizeof(*qu->answer));  if (!qu->answer) { free(qu); return 0; }

  qu->ads= ads;
  qu->state= query_tosend;
  qu->back= qu->next= qu->parent= 0;
  LIST_INIT(qu->children);
  LINK_INIT(qu->siblings);
  LIST_INIT(qu->allocations);
  qu->interim_allocd= 0;
  qu->preserved_allocd= 0;
  qu->final_allocspace= 0;

  qu->typei= typei;
  qu->query_dgram= 0;
  qu->query_dglen= 0;
  adns__vbuf_init(&qu->vb);

  qu->cname_dgram= 0;
  qu->cname_dglen= qu->cname_begin= 0;

  adns__vbuf_init(&qu->search_vb);
  qu->search_origlen= qu->search_pos= qu->search_doneabs= 0;

  qu->id= -2; /* will be overwritten with real id before we leave adns */
  qu->flags= flags;
  qu->retries= 0;
  qu->udpnextserver= 0;
  qu->udpsent= 0;
  timerclear(&qu->timeout);
  qu->expires= now.tv_sec + MAXTTLBELIEVE;

  memset(&qu->ctx,0,sizeof(qu->ctx));

  qu->answer->status= adns_s_ok;
  qu->answer->cname= qu->answer->owner= 0;
  qu->answer->type= typei->type;
  qu->answer->expires= -1;
  qu->answer->nrrs= 0;
  qu->answer->rrs.untyped= 0;
  qu->answer->rrsz= typei->rrsz;

  return qu;
}

static void query_submit(adns_state ads, adns_query qu,
			 const typeinfo *typei, vbuf *qumsg_vb, int id,
			 adns_queryflags flags, struct timeval now) {
  /* Fills in the query message in for a previously-allocated query,
   * and submits it.  Cannot fail.  Takes over the memory for qumsg_vb.
   */

  qu->vb= *qumsg_vb;
  adns__vbuf_init(qumsg_vb);

  qu->query_dgram= malloc( (size_t) qu->vb.used);
  if (!qu->query_dgram) { adns__query_fail(qu,adns_s_nomemory); return; }

  qu->id= id;
  qu->query_dglen= qu->vb.used;
  memcpy(qu->query_dgram,qu->vb.buf,(size_t) qu->vb.used);

  adns__query_send(qu,now);
}

adns_status adns__internal_submit(adns_state ads, adns_query *query_r,
				  const typeinfo *typei, vbuf *qumsg_vb, int id,
				  adns_queryflags flags, struct timeval now,
				  const qcontext *ctx) {
  adns_query qu;

  qu= query_alloc(ads,typei,flags,now);
  if (!qu) { adns__vbuf_free(qumsg_vb); return adns_s_nomemory; }
  *query_r= qu;

  memcpy(&qu->ctx,ctx,(size_t) sizeof(qu->ctx));
  query_submit(ads,qu, typei,qumsg_vb,id,flags,now);

  return adns_s_ok;
}

static void query_simple(adns_state ads, adns_query qu,
			 const char *owner, int ol,
			 const typeinfo *typei, adns_queryflags flags,
			 struct timeval now) {
  vbuf vb_new;
  int id;
  adns_status stat;

  stat= adns__mkquery(ads,&qu->vb,&id, owner,ol, typei,flags);
  if (stat) {
    if (stat == adns_s_querydomaintoolong && (flags & adns_qf_search)) {
      adns__search_next(ads,qu,now);
      return;
    } else {
      adns__query_fail(qu,stat);
      return;
    }
  }

  vb_new= qu->vb;
  adns__vbuf_init(&qu->vb);
  query_submit(ads,qu, typei,&vb_new,id, flags,now);
}

void adns__search_next(adns_state ads, adns_query qu, struct timeval now) {
  const char *nextentry;
  adns_status stat;

  if (qu->search_doneabs<0) {
    nextentry= 0;
    qu->search_doneabs= 1;
  } else {
    if (qu->search_pos >= ads->nsearchlist) {
      if (qu->search_doneabs) {
	stat= adns_s_nxdomain; goto x_fail;
	return;
      } else {
	nextentry= 0;
	qu->search_doneabs= 1;
      }
    } else {
      nextentry= ads->searchlist[qu->search_pos++];
    }
  }

  qu->search_vb.used= qu->search_origlen;
  if (nextentry) {
    if (!adns__vbuf_append(&qu->search_vb,(byte*)".",1) ||
	!adns__vbuf_appendstr(&qu->search_vb,nextentry)) {
      stat= adns_s_nomemory; goto x_fail;
    }
  }

  free(qu->query_dgram);
  qu->query_dgram= 0; qu->query_dglen= 0;

  query_simple(ads,qu, (char*)qu->search_vb.buf, qu->search_vb.used, qu->typei, qu->flags, now);
  return;

x_fail:
  adns__query_fail(qu,stat);
}

static int save_owner(adns_query qu, const char *owner, int ol) {
  /* Returns 1 if OK, otherwise there was no memory. */
  adns_answer *ans;

  ans= qu->answer;
  assert(!ans->owner);

  ans->owner= adns__alloc_preserved(qu, (size_t) ol+1);  if (!ans->owner) return 0;

  memcpy(ans->owner,owner, (size_t) ol);
  ans->owner[ol]= 0;
  return 1;
}

int adns_submit(adns_state ads,
		const char *owner,
		adns_rrtype type,
		adns_queryflags flags,
		void *context,
		adns_query *query_r) {
  int r, ol, ndots;
  adns_status stat;
  const typeinfo *typei;
  struct timeval now;
  adns_query qu;
  const char *p;

  adns__consistency(ads,0,cc_entex);

  typei= adns__findtype(type);
  if (!typei) return ENOSYS;

  r= gettimeofday(&now,0); if (r) goto x_errno;
  qu= query_alloc(ads,typei,flags,now); if (!qu) goto x_errno;

  qu->ctx.ext= context;
  qu->ctx.callback= 0;
  memset(&qu->ctx.info,0,sizeof(qu->ctx.info));

  *query_r= qu;

  ol= strlen(owner);
  if (!ol) { stat= adns_s_querydomaininvalid; goto x_adnsfail; }
  if (ol>DNS_MAXDOMAIN+1) { stat= adns_s_querydomaintoolong; goto x_adnsfail; }

  if (ol>=1 && owner[ol-1]=='.' && (ol<2 || owner[ol-2]!='\\')) {
    flags &= ~adns_qf_search;
    qu->flags= flags;
    ol--;
  }

  if (flags & adns_qf_search) {
    r= adns__vbuf_append(&qu->search_vb,(byte*)owner,ol);
    if (!r) { stat= adns_s_nomemory; goto x_adnsfail; }

    for (ndots=0, p=owner; (p= strchr(p,'.')); p++, ndots++);
    qu->search_doneabs= (ndots >= ads->searchndots) ? -1 : 0;
    qu->search_origlen= ol;
    adns__search_next(ads,qu,now);
  } else {
    if (flags & adns_qf_owner) {
      if (!save_owner(qu,owner,ol)) { stat= adns_s_nomemory; goto x_adnsfail; }
    }
    query_simple(ads,qu, owner,ol, typei,flags, now);
  }
  adns__autosys(ads,now);
  adns__consistency(ads,qu,cc_entex);
  return 0;

 x_adnsfail:
  adns__query_fail(qu,stat);
  adns__consistency(ads,qu,cc_entex);
  return 0;

 x_errno:
  r= errno;
  assert(r);
  adns__consistency(ads,0,cc_entex);
  return r;
}

int adns_submit_reverse_any(adns_state ads,
			    const struct sockaddr *addr,
			    const char *zone,
			    adns_rrtype type,
			    adns_queryflags flags,
			    void *context,
			    adns_query *query_r) {
  const unsigned char *iaddr;
  char *buf, *buf_free;
  char shortbuf[100];
  int r, lreq;

  flags &= ~adns_qf_search;

  if (addr->sa_family != AF_INET) return ENOSYS;
  iaddr= (const unsigned char*) &(((const struct sockaddr_in*)addr) -> sin_addr);

  lreq= strlen(zone) + 4*4 + 1;
  if (lreq > (int)sizeof(shortbuf)) {
    buf= malloc( strlen(zone) + 4*4 + 1 );
    if (!buf) return errno;
    buf_free= buf;
  } else {
    buf= shortbuf;
    buf_free= 0;
  }
  sprintf(buf, "%d.%d.%d.%d.%s", iaddr[3], iaddr[2], iaddr[1], iaddr[0], zone);

  r= adns_submit(ads,buf,type,flags,context,query_r);
  free(buf_free);
  return r;
}

int adns_submit_reverse(adns_state ads,
			const struct sockaddr *addr,
			adns_rrtype type,
			adns_queryflags flags,
			void *context,
			adns_query *query_r) {
  if (type != adns_r_ptr && type != adns_r_ptr_raw) return EINVAL;
  return adns_submit_reverse_any(ads,addr,"in-addr.arpa",type,flags,context,query_r);
}

int adns_synchronous(adns_state ads,
		     const char *owner,
		     adns_rrtype type,
		     adns_queryflags flags,
		     adns_answer **answer_r) {
  adns_query qu;
  int r;

  r= adns_submit(ads,owner,type,flags,0,&qu);
  if (r) return r;

  r= adns_wait(ads,&qu,answer_r,0);
  if (r) adns_cancel(qu);

  return r;
}

static void *alloc_common(adns_query qu, size_t sz) {
  allocnode *an;

  if (!sz) return qu; /* Any old pointer will do */
  assert(!qu->final_allocspace);
  an= malloc(MEM_ROUND(MEM_ROUND(sizeof(*an)) + sz));
  if (!an) return 0;
  LIST_LINK_TAIL(qu->allocations,an);
  return (byte*)an + MEM_ROUND(sizeof(*an));
}

void *adns__alloc_interim(adns_query qu, size_t sz) {
  void *rv;

  sz= MEM_ROUND(sz);
  rv= alloc_common(qu,sz);
  if (!rv) return 0;
  qu->interim_allocd += sz;
  return rv;
}

void *adns__alloc_preserved(adns_query qu, size_t sz) {
  void *rv;

  sz= MEM_ROUND(sz);
  rv= adns__alloc_interim(qu,sz);
  if (!rv) return 0;
  qu->preserved_allocd += sz;
  return rv;
}

void *adns__alloc_mine(adns_query qu, size_t sz) {
  return alloc_common(qu,MEM_ROUND(sz));
}

void adns__transfer_interim(adns_query from, adns_query to, void *block, size_t sz) {
  allocnode *an;

  if (!block) return;
  an= (void*)((byte*)block - MEM_ROUND(sizeof(*an)));

  assert(!to->final_allocspace);
  assert(!from->final_allocspace);

  LIST_UNLINK(from->allocations,an);
  LIST_LINK_TAIL(to->allocations,an);

  sz= MEM_ROUND(sz);
  from->interim_allocd -= sz;
  to->interim_allocd += sz;

  if (to->expires > from->expires) to->expires= from->expires;
}

void *adns__alloc_final(adns_query qu, size_t sz) {
  /* When we're in the _final stage, we _subtract_ from interim_alloc'd
   * each allocation, and use final_allocspace to point to the next free
   * bit.
   */
  void *rp;

  sz= MEM_ROUND(sz);
  rp= qu->final_allocspace;
  assert(rp);
  qu->interim_allocd -= sz;
  assert(qu->interim_allocd>=0);
  qu->final_allocspace= (byte*)rp + sz;
  return rp;
}

static void cancel_children(adns_query qu) {
  adns_query cqu, ncqu;

  for (cqu= qu->children.head; cqu; cqu= ncqu) {
    ncqu= cqu->siblings.next;
    adns_cancel(cqu);
  }
}

void adns__reset_preserved(adns_query qu) {
  assert(!qu->final_allocspace);
  cancel_children(qu);
  qu->answer->nrrs= 0;
  qu->answer->rrs.untyped= 0;
  qu->interim_allocd= qu->preserved_allocd;
}

static void free_query_allocs(adns_query qu) {
  allocnode *an, *ann;

  cancel_children(qu);
  for (an= qu->allocations.head; an; an= ann) { ann= an->next; free(an); }
  LIST_INIT(qu->allocations);
  adns__vbuf_free(&qu->vb);
  adns__vbuf_free(&qu->search_vb);
  free(qu->query_dgram);
  qu->query_dgram= 0;
}

void adns_cancel(adns_query qu) {
  adns_state ads;

  ads= qu->ads;
  adns__consistency(ads,qu,cc_entex);
  if (qu->parent) LIST_UNLINK_PART(qu->parent->children,qu,siblings.);
  switch (qu->state) {
  case query_tosend:
    LIST_UNLINK(ads->udpw,qu);
    break;
  case query_tcpw:
    LIST_UNLINK(ads->tcpw,qu);
    break;
  case query_childw:
    LIST_UNLINK(ads->childw,qu);
    break;
  case query_done:
    LIST_UNLINK(ads->output,qu);
    break;
  default:
    abort();
  }
  free_query_allocs(qu);
  free(qu->answer);
  free(qu);
  adns__consistency(ads,0,cc_entex);
}

void adns__update_expires(adns_query qu, unsigned long ttl, struct timeval now) {
  time_t max;

  assert(ttl <= MAXTTLBELIEVE);
  max= now.tv_sec + ttl;
  if (qu->expires < max) return;
  qu->expires= max;
}

static void makefinal_query(adns_query qu) {
  adns_answer *ans;
  int rrn;

  ans= qu->answer;

  if (qu->interim_allocd) {
    ans= realloc(qu->answer, MEM_ROUND(MEM_ROUND(sizeof(*ans)) + qu->interim_allocd));
    if (!ans) goto x_nomem;
    qu->answer= ans;
  }

  qu->final_allocspace= (byte*)ans + MEM_ROUND(sizeof(*ans));
  adns__makefinal_str(qu,&ans->cname);
  adns__makefinal_str(qu,&ans->owner);

  if (ans->nrrs) {
    adns__makefinal_block(qu, &ans->rrs.untyped, (size_t) ans->nrrs*ans->rrsz);

    for (rrn=0; rrn<ans->nrrs; rrn++)
      qu->typei->makefinal(qu, ans->rrs.bytes + rrn*ans->rrsz);
  }

  free_query_allocs(qu);
  return;

 x_nomem:
  qu->preserved_allocd= 0;
  qu->answer->cname= 0;
  qu->answer->owner= 0;
  adns__reset_preserved(qu); /* (but we just threw away the preserved stuff) */

  qu->answer->status= adns_s_nomemory;
  free_query_allocs(qu);
}

void adns__query_done(adns_query qu) {
  adns_answer *ans;
  adns_query parent;

  cancel_children(qu);

  qu->id= -1;
  ans= qu->answer;

  if (qu->flags & adns_qf_owner && qu->flags & adns_qf_search &&
      ans->status != adns_s_nomemory) {
    if (!save_owner(qu, (char*)qu->search_vb.buf, qu->search_vb.used)) {
      adns__query_fail(qu,adns_s_nomemory);
      return;
    }
  }

  if (ans->nrrs && qu->typei->diff_needswap) {
    if (!adns__vbuf_ensure(&qu->vb,qu->typei->rrsz)) {
      adns__query_fail(qu,adns_s_nomemory);
      return;
    }
    adns__isort(ans->rrs.bytes, ans->nrrs, ans->rrsz,
		qu->vb.buf,
		(int(*)(void*, const void*, const void*))qu->typei->diff_needswap,
		qu->ads);
  }

  ans->expires= qu->expires;
  parent= qu->parent;
  if (parent) {
    LIST_UNLINK_PART(parent->children,qu,siblings.);
    LIST_UNLINK(qu->ads->childw,parent);
    qu->ctx.callback(parent,qu);
    free_query_allocs(qu);
    free(qu->answer);
    free(qu);
  } else {
    makefinal_query(qu);
    LIST_LINK_TAIL(qu->ads->output,qu);
    qu->state= query_done;
  }
}

void adns__query_fail(adns_query qu, adns_status stat) {
  adns__reset_preserved(qu);
  qu->answer->status= stat;
  adns__query_done(qu);
}

void adns__makefinal_str(adns_query qu, char **strp) {
  int l;
  char *before, *after;

  before= *strp;
  if (!before) return;
  l= strlen(before)+1;
  after= adns__alloc_final(qu, (size_t) l);
  memcpy(after,before,(size_t) l);
  *strp= after;
}

void adns__makefinal_block(adns_query qu, void **blpp, size_t sz) {
  void *before, *after;

  before= *blpp;
  if (!before) return;
  after= adns__alloc_final(qu,sz);
  memcpy(after,before, (size_t) sz);
  *blpp= after;
}
