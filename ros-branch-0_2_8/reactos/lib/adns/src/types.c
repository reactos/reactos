/*
 * types.c
 * - RR-type-specific code, and the machinery to call it
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

#ifdef ADNS_JGAA_WIN32
# include "adns_win32.h"
#else
# include <stdlib.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

#include "internal.h"

#define R_NOMEM           return adns_s_nomemory
#define CSP_ADDSTR(s)     do { if (!adns__vbuf_appendstr(vb,(s))) R_NOMEM; } while (0)

/*
 * order of sections:
 *
 * _string                    (pap)
 * _textdata, _qstring        (csp)
 * _str                       (mf,cs)
 * _intstr                    (mf,csp,cs)
 * _manyistr                  (mf,cs)
 * _txt                       (pa)
 * _inaddr                    (pa,dip,di)
 * _addr                      (pa,di,csp,cs)
 * _domain                    (pap)
 * _host_raw                  (pa)
 * _hostaddr                  (pap,pa,dip,di,mfp,mf,csp,cs +pap_findaddrs)
 * _mx_raw                    (pa,di)
 * _mx                        (pa,di)
 * _inthostaddr               (mf,cs)
 * _ptr                       (pa)
 * _strpair                   (mf,cs)
 * _intstrpair                (mf,cs)
 * _hinfo                     (pa)
 * _mailbox                   (pap +pap_mailbox822)
 * _rp                        (pa)
 * _soa                       (pa,mf,cs)
 * _flat                      (mf)
 *
 * within each section:
 *    pap_*
 *    pa_*
 *    dip_*
 *    di_*
 *    mfp_*
 *    mf_*
 *    csp_*
 *    cs_*
 */

/*
 * _qstring               (pap,csp)
 */

static adns_status pap_qstring(const parseinfo *pai, int *cbyte_io, int max,
			      int *len_r, char **str_r) {
  /* Neither len_r nor str_r may be null.
   * End of datagram (overrun) is indicated by returning adns_s_invaliddata;
   */
  const byte *dgram= pai->dgram;
  int l, cbyte;
  char *str;

  cbyte= *cbyte_io;

  if (cbyte >= max) return adns_s_invaliddata;
  GET_B(cbyte,l);
  if (cbyte+l > max) return adns_s_invaliddata;
  
  str= adns__alloc_interim(pai->qu, (size_t)l+1);
  if (!str) R_NOMEM;
  
  str[l]= 0;
  memcpy(str,dgram+cbyte,(size_t)l);

  *len_r= l;
  *str_r= str;
  *cbyte_io= cbyte+l;
  
  return adns_s_ok;
}

static adns_status csp_qstring(vbuf *vb, const char *dp, int len) {
  unsigned char ch;
  char buf[10];
  int cn;

  CSP_ADDSTR("\"");
  for (cn=0; cn<len; cn++) {
    ch= *dp++;
    if (ch == '\\') {
      CSP_ADDSTR("\\\\");
    } else if (ch == '"') {
      CSP_ADDSTR("\\\"");
    } else if (ch >= 32 && ch <= 126) {
      if (!adns__vbuf_append(vb,&ch,1)) R_NOMEM;
    } else {
      sprintf(buf,"\\x%02x",ch);
      CSP_ADDSTR(buf);
    }
  }
  CSP_ADDSTR("\"");
  
  return adns_s_ok;
}

/*
 * _str  (mf)
 */

static void mf_str(adns_query qu, void *datap) {
  char **rrp= datap;

  adns__makefinal_str(qu,rrp);
}

/*
 * _intstr  (mf)
 */

static void mf_intstr(adns_query qu, void *datap) {
  adns_rr_intstr *rrp= datap;

  adns__makefinal_str(qu,&rrp->str);
}

/*
 * _manyistr   (mf)
 */

static void mf_manyistr(adns_query qu, void *datap) {
  adns_rr_intstr **rrp= datap;
  adns_rr_intstr *te, *table;
  void *tablev;
  int tc;

  for (tc=0, te= *rrp; te->i >= 0; te++, tc++);
  tablev= *rrp;
  adns__makefinal_block(qu,&tablev,sizeof(*te)*(tc+1));
  *rrp= table= tablev;
  for (te= *rrp; te->i >= 0; te++)
    adns__makefinal_str(qu,&te->str);
}

/*
 * _txt   (pa,cs)
 */

static adns_status pa_txt(const parseinfo *pai, int cbyte, int max, void *datap) {
  adns_rr_intstr **rrp= datap, *table, *te;
  const byte *dgram= pai->dgram;
  int ti, tc, l, startbyte;
  adns_status st;

  startbyte= cbyte;
  if (cbyte >= max) return adns_s_invaliddata;
  tc= 0;
  while (cbyte < max) {
    GET_B(cbyte,l);
    cbyte+= l;
    tc++;
  }
  if (cbyte != max || !tc) return adns_s_invaliddata;

  table= adns__alloc_interim(pai->qu,sizeof(*table)*(tc+1));
  if (!table) R_NOMEM;

  for (cbyte=startbyte, ti=0, te=table; ti<tc; ti++, te++) {
    st= pap_qstring(pai, &cbyte, max, &te->i, &te->str);
    if (st) return st;
  }
  assert(cbyte == max);

  te->i= -1;
  te->str= 0;
  
  *rrp= table;
  return adns_s_ok;
}

static adns_status cs_txt(vbuf *vb, const void *datap) {
  const adns_rr_intstr *const *rrp= datap;
  const adns_rr_intstr *current;
  adns_status st;
  int spc;

  for (current= *rrp, spc=0;  current->i >= 0;  current++, spc=1) {
    if (spc) CSP_ADDSTR(" ");
    st= csp_qstring(vb,current->str,current->i); if (st) return st;
  }
  return adns_s_ok;
}

/*
 * _hinfo   (cs)
 */

static adns_status cs_hinfo(vbuf *vb, const void *datap) {
  const adns_rr_intstrpair *rrp= datap;
  adns_status st;

  st= csp_qstring(vb,rrp->array[0].str,rrp->array[0].i);  if (st) return st;
  CSP_ADDSTR(" ");
  st= csp_qstring(vb,rrp->array[1].str,rrp->array[1].i);  if (st) return st;
  return adns_s_ok;
}

/*
 * _inaddr   (pa,dip,di)
 */

static adns_status pa_inaddr(const parseinfo *pai, int cbyte, int max, void *datap) {
  struct in_addr *storeto= datap;
  
  if (max-cbyte != 4) return adns_s_invaliddata;
  memcpy(storeto, pai->dgram + cbyte, 4);
  return adns_s_ok;
}

static int search_sortlist(adns_state ads, struct in_addr ad) {
  const struct sortlist *slp;
  int i;
  
  for (i=0, slp=ads->sortlist;
       i<ads->nsortlist && !((ad.s_addr & slp->mask.s_addr) == slp->base.s_addr);
       i++, slp++);
  return i;
}

static int dip_inaddr(adns_state ads, struct in_addr a, struct in_addr b) {
  int ai, bi;
  
  if (!ads->nsortlist) return 0;

  ai= search_sortlist(ads,a);
  bi= search_sortlist(ads,b);
  return bi<ai;
}

static int di_inaddr(adns_state ads, const void *datap_a, const void *datap_b) {
  const struct in_addr *ap= datap_a, *bp= datap_b;

  return dip_inaddr(ads,*ap,*bp);
}

static adns_status cs_inaddr(vbuf *vb, const void *datap) {
  const struct in_addr *rrp= datap, rr= *rrp;
  const char *ia;

  ia= inet_ntoa(rr); assert(ia);
  CSP_ADDSTR(ia);
  return adns_s_ok;
}

/*
 * _addr   (pa,di,csp,cs)
 */

static adns_status pa_addr(const parseinfo *pai, int cbyte, int max, void *datap) {
  adns_rr_addr *storeto= datap;
  const byte *dgram= pai->dgram;

  if (max-cbyte != 4) return adns_s_invaliddata;
  storeto->len= sizeof(storeto->addr.inet);
  memset(&storeto->addr,0,sizeof(storeto->addr.inet));
  storeto->addr.inet.sin_family= AF_INET;
  memcpy(&storeto->addr.inet.sin_addr,dgram+cbyte,4);
  return adns_s_ok;
}

static int di_addr(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_addr *ap= datap_a, *bp= datap_b;

  assert(ap->addr.sa.sa_family == AF_INET);
  return dip_inaddr(ads, ap->addr.inet.sin_addr, bp->addr.inet.sin_addr);
}

static int div_addr(void *context, const void *datap_a, const void *datap_b) {
  const adns_state ads= context;

  return di_addr(ads, datap_a, datap_b);
}		     

static adns_status csp_addr(vbuf *vb, const adns_rr_addr *rrp) {
  const char *ia;
  static char buf[30];

  switch (rrp->addr.inet.sin_family) {
  case AF_INET:
    CSP_ADDSTR("INET ");
    ia= inet_ntoa(rrp->addr.inet.sin_addr); assert(ia);
    CSP_ADDSTR(ia);
    break;
  default:
    sprintf(buf,"AF=%u",rrp->addr.sa.sa_family);
    CSP_ADDSTR(buf);
    break;
  }
  return adns_s_ok;
}

static adns_status cs_addr(vbuf *vb, const void *datap) {
  const adns_rr_addr *rrp= datap;

  return csp_addr(vb,rrp);
}

/*
 * _domain      (pap,csp,cs)
 * _dom_raw     (pa)
 */

static adns_status pap_domain(const parseinfo *pai, int *cbyte_io, int max,
			      char **domain_r, parsedomain_flags flags) {
  adns_status st;
  char *dm;
  
  st= adns__parse_domain(pai->qu->ads, pai->serv, pai->qu, &pai->qu->vb, flags,
			 pai->dgram,pai->dglen, cbyte_io, max);
  if (st) return st;
  if (!pai->qu->vb.used) return adns_s_invaliddata;

  dm= adns__alloc_interim(pai->qu, (size_t) pai->qu->vb.used+1);
  if (!dm) R_NOMEM;

  dm[pai->qu->vb.used]= 0;
  memcpy(dm,pai->qu->vb.buf, (size_t) pai->qu->vb.used);
  
  *domain_r= dm;
  return adns_s_ok;
}

static adns_status csp_domain(vbuf *vb, const char *domain) {
  CSP_ADDSTR(domain);
  if (!*domain) CSP_ADDSTR(".");
  return adns_s_ok;
}

static adns_status cs_domain(vbuf *vb, const void *datap) {
  const char *const *domainp= datap;
  return csp_domain(vb,*domainp);
}

static adns_status pa_dom_raw(const parseinfo *pai, int cbyte, int max, void *datap) {
  char **rrp= datap;
  adns_status st;

  st= pap_domain(pai, &cbyte, max, rrp, pdf_quoteok);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

/*
 * _host_raw   (pa)
 */

static adns_status pa_host_raw(const parseinfo *pai, int cbyte, int max, void *datap) {
  char **rrp= datap;
  adns_status st;

  st= pap_domain(pai, &cbyte, max, rrp,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

/*
 * _hostaddr   (pap,pa,dip,di,mfp,mf,csp,cs +icb_hostaddr, pap_findaddrs)
 */

static adns_status pap_findaddrs(const parseinfo *pai, adns_rr_hostaddr *ha,
				 int *cbyte_io, int count, int dmstart) {
  int rri, naddrs;
  int type, class, rdlen, rdstart, ownermatched;
  unsigned long ttl;
  adns_status st;
  
  for (rri=0, naddrs=-1; rri<count; rri++) {
    st= adns__findrr_anychk(pai->qu, pai->serv, pai->dgram, pai->dglen, cbyte_io,
			    &type, &class, &ttl, &rdlen, &rdstart,
			    pai->dgram, pai->dglen, dmstart, &ownermatched);
    if (st) return st;
    if (!ownermatched || class != DNS_CLASS_IN || type != adns_r_a) {
      if (naddrs>0) break; else continue;
    }
    if (naddrs == -1) {
      naddrs= 0;
    }
    if (!adns__vbuf_ensure(&pai->qu->vb, (int) ((naddrs+1)*sizeof(adns_rr_addr)))) R_NOMEM;
    adns__update_expires(pai->qu,ttl,pai->now);
    st= pa_addr(pai, rdstart,rdstart+rdlen,
		pai->qu->vb.buf + naddrs*sizeof(adns_rr_addr));
    if (st) return st;
    naddrs++;
  }
  if (naddrs >= 0) {
    ha->addrs= adns__alloc_interim(pai->qu, naddrs*sizeof(adns_rr_addr));
    if (!ha->addrs) R_NOMEM;
    memcpy(ha->addrs, pai->qu->vb.buf, naddrs*sizeof(adns_rr_addr));
    ha->naddrs= naddrs;
    ha->astatus= adns_s_ok;

    adns__isort(ha->addrs, naddrs, sizeof(adns_rr_addr), pai->qu->vb.buf,
		div_addr, pai->ads);
  }
  return adns_s_ok;
}

static void icb_hostaddr(adns_query parent, adns_query child) {
  adns_answer *cans= child->answer;
  adns_rr_hostaddr *rrp= child->ctx.info.hostaddr;
  adns_state ads= parent->ads;
  adns_status st;

  st= cans->status;
  rrp->astatus= st;
  rrp->naddrs= (st>0 && st<=adns_s_max_tempfail) ? -1 : cans->nrrs;
  rrp->addrs= cans->rrs.addr;
  adns__transfer_interim(child, parent, rrp->addrs, rrp->naddrs*sizeof(adns_rr_addr));

  if (parent->children.head) {
    LIST_LINK_TAIL(ads->childw,parent);
  } else {
    adns__query_done(parent);
  }
}

static adns_status pap_hostaddr(const parseinfo *pai, int *cbyte_io,
				int max, adns_rr_hostaddr *rrp) {
  adns_status st;
  int dmstart, cbyte;
  qcontext ctx;
  int id;
  adns_query nqu;
  adns_queryflags nflags;

  dmstart= cbyte= *cbyte_io;
  st= pap_domain(pai, &cbyte, max, &rrp->host,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  *cbyte_io= cbyte;

  rrp->astatus= adns_s_ok;
  rrp->naddrs= -1;
  rrp->addrs= 0;

  cbyte= pai->nsstart;

  st= pap_findaddrs(pai, rrp, &cbyte, pai->nscount, dmstart);
  if (st) return st;
  if (rrp->naddrs != -1) return adns_s_ok;

  st= pap_findaddrs(pai, rrp, &cbyte, pai->arcount, dmstart);
  if (st) return st;
  if (rrp->naddrs != -1) return adns_s_ok;

  st= adns__mkquery_frdgram(pai->ads, &pai->qu->vb, &id,
			    pai->dgram, pai->dglen, dmstart,
			    adns_r_addr, adns_qf_quoteok_query);
  if (st) return st;

  ctx.ext= 0;
  ctx.callback= icb_hostaddr;
  ctx.info.hostaddr= rrp;
  
  nflags= adns_qf_quoteok_query;
  if (!(pai->qu->flags & adns_qf_cname_loose)) nflags |= adns_qf_cname_forbid;
  
  st= adns__internal_submit(pai->ads, &nqu, adns__findtype(adns_r_addr),
			    &pai->qu->vb, id, nflags, pai->now, &ctx);
  if (st) return st;

  nqu->parent= pai->qu;
  LIST_LINK_TAIL_PART(pai->qu->children,nqu,siblings.);

  return adns_s_ok;
}

static adns_status pa_hostaddr(const parseinfo *pai, int cbyte, int max, void *datap) {
  adns_rr_hostaddr *rrp= datap;
  adns_status st;

  st= pap_hostaddr(pai, &cbyte, max, rrp);
  if (st) return st;
  if (cbyte != max) return adns_s_invaliddata;

  return adns_s_ok;
}

static int dip_hostaddr(adns_state ads, const adns_rr_hostaddr *ap, const adns_rr_hostaddr *bp) {
  if (ap->astatus != bp->astatus) return ap->astatus;
  if (ap->astatus) return 0;

  assert(ap->addrs[0].addr.sa.sa_family == AF_INET);
  assert(bp->addrs[0].addr.sa.sa_family == AF_INET);
  return dip_inaddr(ads,
		    ap->addrs[0].addr.inet.sin_addr,
		    bp->addrs[0].addr.inet.sin_addr);
}

static int di_hostaddr(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_hostaddr *ap= datap_a, *bp= datap_b;

  return dip_hostaddr(ads, ap,bp);
}

static void mfp_hostaddr(adns_query qu, adns_rr_hostaddr *rrp) {
  void *tablev;

  adns__makefinal_str(qu,&rrp->host);
  tablev= rrp->addrs;
  adns__makefinal_block(qu, &tablev, rrp->naddrs*sizeof(*rrp->addrs));
  rrp->addrs= tablev;
}

static void mf_hostaddr(adns_query qu, void *datap) {
  adns_rr_hostaddr *rrp= datap;

  mfp_hostaddr(qu,rrp);
}

static adns_status csp_hostaddr(vbuf *vb, const adns_rr_hostaddr *rrp) {
  const char *errstr;
  adns_status st;
  char buf[20];
  int i;

  st= csp_domain(vb,rrp->host);  if (st) return st;

  CSP_ADDSTR(" ");
  CSP_ADDSTR(adns_errtypeabbrev(rrp->astatus));

  sprintf(buf," %d ",rrp->astatus);
  CSP_ADDSTR(buf);

  CSP_ADDSTR(adns_errabbrev(rrp->astatus));
  CSP_ADDSTR(" ");

  errstr= adns_strerror(rrp->astatus);
  st= csp_qstring(vb,errstr,(int)strlen(errstr));  if (st) return st;
  
  if (rrp->naddrs >= 0) {
    CSP_ADDSTR(" (");
    for (i=0; i<rrp->naddrs; i++) {
      CSP_ADDSTR(" ");
      st= csp_addr(vb,&rrp->addrs[i]);
    }
    CSP_ADDSTR(" )");
  } else {
    CSP_ADDSTR(" ?");
  }
  return adns_s_ok;
}

static adns_status cs_hostaddr(vbuf *vb, const void *datap) {
  const adns_rr_hostaddr *rrp= datap;

  return csp_hostaddr(vb,rrp);
}

/*
 * _mx_raw   (pa,di)
 */

static adns_status pa_mx_raw(const parseinfo *pai, int cbyte, int max, void *datap) {
  const byte *dgram= pai->dgram;
  adns_rr_intstr *rrp= datap;
  adns_status st;
  int pref;

  if (cbyte+2 > max) return adns_s_invaliddata;
  GET_W(cbyte,pref);
  rrp->i= pref;
  st= pap_domain(pai, &cbyte, max, &rrp->str,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static int di_mx_raw(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_intstr *ap= datap_a, *bp= datap_b;

  if (ap->i < bp->i) return 0;
  if (ap->i > bp->i) return 1;
  return 0;
}

/*
 * _mx   (pa,di)
 */

static adns_status pa_mx(const parseinfo *pai, int cbyte, int max, void *datap) {
  const byte *dgram= pai->dgram;
  adns_rr_inthostaddr *rrp= datap;
  adns_status st;
  int pref;

  if (cbyte+2 > max) return adns_s_invaliddata;
  GET_W(cbyte,pref);
  rrp->i= pref;
  st= pap_hostaddr(pai, &cbyte, max, &rrp->ha);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static int di_mx(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_inthostaddr *ap= datap_a, *bp= datap_b;

  if (ap->i < bp->i) return 0;
  if (ap->i > bp->i) return 1;
  return dip_hostaddr(ads, &ap->ha, &bp->ha);
}

/*
 * _inthostaddr  (mf,cs)
 */

static void mf_inthostaddr(adns_query qu, void *datap) {
  adns_rr_inthostaddr *rrp= datap;

  mfp_hostaddr(qu,&rrp->ha);
}

static adns_status cs_inthostaddr(vbuf *vb, const void *datap) {
  const adns_rr_inthostaddr *rrp= datap;
  char buf[10];

  sprintf(buf,"%u ",rrp->i);
  CSP_ADDSTR(buf);

  return csp_hostaddr(vb,&rrp->ha);
}

/*
 * _inthost  (cs)
 */

static adns_status cs_inthost(vbuf *vb, const void *datap) {
  const adns_rr_intstr *rrp= datap;
  char buf[10];

  sprintf(buf,"%u ",rrp->i);
  CSP_ADDSTR(buf);
  return csp_domain(vb,rrp->str);
}

/*
 * _ptr   (pa, +icb_ptr)
 */

static void icb_ptr(adns_query parent, adns_query child) {
  adns_answer *cans= child->answer;
  const adns_rr_addr *queried, *found;
  adns_state ads= parent->ads;
  int i;

  if (cans->status == adns_s_nxdomain || cans->status == adns_s_nodata) {
    adns__query_fail(parent,adns_s_inconsistent);
    return;
  } else if (cans->status) {
    adns__query_fail(parent,cans->status);
    return;
  }

  queried= &parent->ctx.info.ptr_parent_addr;
  for (i=0, found=cans->rrs.addr; i<cans->nrrs; i++, found++) {
    if (queried->len == found->len &&
	!memcmp(&queried->addr,&found->addr,(size_t) queried->len)) {
      if (!parent->children.head) {
	adns__query_done(parent);
	return;
      } else {
	LIST_LINK_TAIL(ads->childw,parent);
	return;
      }
    }
  }

  adns__query_fail(parent,adns_s_inconsistent);
}

static adns_status pa_ptr(const parseinfo *pai, int dmstart, int max, void *datap) {
  static const char *(expectdomain[])= { DNS_INADDR_ARPA };
  
  char **rrp= datap;
  adns_status st;
  adns_rr_addr *ap;
  findlabel_state fls;
  char *ep;
  byte ipv[4];
  char labbuf[4];
  int cbyte, i, lablen, labstart, l, id;
  adns_query nqu;
  qcontext ctx;

  cbyte= dmstart;
  st= pap_domain(pai, &cbyte, max, rrp,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  if (cbyte != max) return adns_s_invaliddata;

  ap= &pai->qu->ctx.info.ptr_parent_addr;
  if (!ap->len) {
    adns__findlabel_start(&fls, pai->ads, -1, pai->qu,
			  pai->qu->query_dgram, pai->qu->query_dglen,
			  pai->qu->query_dglen, DNS_HDRSIZE, 0);
    for (i=0; i<4; i++) {
      st= adns__findlabel_next(&fls,&lablen,&labstart); assert(!st);
      if (lablen<=0 || lablen>3) return adns_s_querydomainwrong;
      memcpy(labbuf, pai->qu->query_dgram + labstart, (size_t) lablen);  labbuf[lablen]= 0;
      ipv[3-i]= (unsigned char)strtoul(labbuf,&ep,10);  if (*ep) return adns_s_querydomainwrong;
      if (lablen>1 && pai->qu->query_dgram[labstart]=='0')
	return adns_s_querydomainwrong;
    }
    for (i=0; i<(int)sizeof(expectdomain)/(int)sizeof(*expectdomain); i++) {
      st= adns__findlabel_next(&fls,&lablen,&labstart); assert(!st);
      l= strlen(expectdomain[i]);
      if (lablen != l || memcmp(pai->qu->query_dgram + labstart, expectdomain[i], (size_t)l))
	return adns_s_querydomainwrong;
    }
    st= adns__findlabel_next(&fls,&lablen,0); assert(!st);
    if (lablen) return adns_s_querydomainwrong;
    
    ap->len= sizeof(struct sockaddr_in);
    memset(&ap->addr,0,sizeof(ap->addr.inet));
    ap->addr.inet.sin_family= AF_INET;
    ap->addr.inet.sin_addr.s_addr=
      htonl((u_long)(ipv[0]<<24) | (ipv[1]<<16) | (ipv[2]<<8) | (ipv[3]));
  }

  st= adns__mkquery_frdgram(pai->ads, &pai->qu->vb, &id,
			    pai->dgram, pai->dglen, dmstart,
			    adns_r_addr, adns_qf_quoteok_query);
  if (st) return st;

  ctx.ext= 0;
  ctx.callback= icb_ptr;
  memset(&ctx.info,0,(size_t) sizeof(ctx.info));
  st= adns__internal_submit(pai->ads, &nqu, adns__findtype(adns_r_addr),
			    &pai->qu->vb, id,
			    adns_qf_quoteok_query, pai->now, &ctx);
  if (st) return st;

  nqu->parent= pai->qu;
  LIST_LINK_TAIL_PART(pai->qu->children,nqu,siblings.);
  return adns_s_ok;
}

/*
 * _strpair   (mf)
 */

static void mf_strpair(adns_query qu, void *datap) {
  adns_rr_strpair *rrp= datap;

  adns__makefinal_str(qu,&rrp->array[0]);
  adns__makefinal_str(qu,&rrp->array[1]);
}

/*
 * _intstrpair   (mf)
 */

static void mf_intstrpair(adns_query qu, void *datap) {
  adns_rr_intstrpair *rrp= datap;

  adns__makefinal_str(qu,&rrp->array[0].str);
  adns__makefinal_str(qu,&rrp->array[1].str);
}

/*
 * _hinfo   (pa)
 */

static adns_status pa_hinfo(const parseinfo *pai, int cbyte, int max, void *datap) {
  adns_rr_intstrpair *rrp= datap;
  adns_status st;
  int i;

  for (i=0; i<2; i++) {
    st= pap_qstring(pai, &cbyte, max, &rrp->array[i].i, &rrp->array[i].str);
    if (st) return st;
  }

  if (cbyte != max) return adns_s_invaliddata;
  
  return adns_s_ok;
}

/*
 * _mailbox   (pap,cs)
 */

static adns_status pap_mailbox822(const parseinfo *pai, int *cbyte_io, int max,
				  char **mb_r) {
  int lablen, labstart, i, needquote, c, r, neednorm;
  const unsigned char *p;
  char *str;
  findlabel_state fls;
  adns_status st;
  vbuf *vb;

  vb= &pai->qu->vb;
  vb->used= 0;
  adns__findlabel_start(&fls, pai->ads,
			-1, pai->qu,
			pai->dgram, pai->dglen, max,
			*cbyte_io, cbyte_io);
  st= adns__findlabel_next(&fls,&lablen,&labstart);
  if (!lablen) {
    adns__vbuf_appendstr(vb,".");
    goto x_ok;
  }

  neednorm= 1;
  for (i=0, needquote=0, p= pai->dgram+labstart; i<lablen; i++) {
    c= *p++;
    if ((c&~128) < 32 || (c&~128) == 127) return adns_s_invaliddata;
    if (c == '.' && !neednorm) neednorm= 1;
    else if (c==' ' || c>=127 || ctype_822special(c)) needquote++;
    else neednorm= 0;
  }

  if (needquote || neednorm) {
    r= adns__vbuf_ensure(vb, lablen+needquote+4); if (!r) R_NOMEM;
    adns__vbuf_appendq(vb,(byte*)"\"",1);
    for (i=0, needquote=0, p= pai->dgram+labstart; i<lablen; i++, p++) {
      c= *p;
      if (c == '"' || c=='\\') adns__vbuf_appendq(vb,(byte*)"\\",1);
      adns__vbuf_appendq(vb,p,1);
    }
    adns__vbuf_appendq(vb,(byte*)"\"",1);
  } else {
    r= adns__vbuf_append(vb, pai->dgram+labstart, lablen); if (!r) R_NOMEM;
  }

  r= adns__vbuf_appendstr(vb,"@"); if (!r) R_NOMEM;

  st= adns__parse_domain_more(&fls,pai->ads, pai->qu,vb,0, pai->dgram);
  if (st) return st;

 x_ok:
  str= adns__alloc_interim(pai->qu, (size_t) vb->used+1); if (!str) R_NOMEM;
  memcpy(str,vb->buf,(size_t) vb->used);
  str[vb->used]= 0;
  *mb_r= str;
  return adns_s_ok;
}

static adns_status pap_mailbox(const parseinfo *pai, int *cbyte_io, int max,
			       char **mb_r) {
  if (pai->qu->typei->type & adns__qtf_mail822) {
    return pap_mailbox822(pai, cbyte_io, max, mb_r);
  } else {
    return pap_domain(pai, cbyte_io, max, mb_r, pdf_quoteok);
  }
}

static adns_status csp_mailbox(vbuf *vb, const char *mailbox) {
  return csp_domain(vb,mailbox);
}

/*
 * _rp   (pa,cs)
 */

static adns_status pa_rp(const parseinfo *pai, int cbyte, int max, void *datap) {
  adns_rr_strpair *rrp= datap;
  adns_status st;

  st= pap_mailbox(pai, &cbyte, max, &rrp->array[0]);
  if (st) return st;

  st= pap_domain(pai, &cbyte, max, &rrp->array[1], pdf_quoteok);
  if (st) return st;

  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static adns_status cs_rp(vbuf *vb, const void *datap) {
  const adns_rr_strpair *rrp= datap;
  adns_status st;

  st= csp_mailbox(vb,rrp->array[0]);  if (st) return st;
  CSP_ADDSTR(" ");
  st= csp_domain(vb,rrp->array[1]);  if (st) return st;

  return adns_s_ok;
}  

/*
 * _soa   (pa,mf,cs)
 */

static adns_status pa_soa(const parseinfo *pai, int cbyte, int max, void *datap) {
  adns_rr_soa *rrp= datap;
  const byte *dgram= pai->dgram;
  adns_status st;
  int msw, lsw, i;

  st= pap_domain(pai, &cbyte, max, &rrp->mname,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;

  st= pap_mailbox(pai, &cbyte, max, &rrp->rname);
  if (st) return st;

  if (cbyte+20 != max) return adns_s_invaliddata;
  
  for (i=0; i<5; i++) {
    GET_W(cbyte,msw);
    GET_W(cbyte,lsw);
    (&rrp->serial)[i]= (msw<<16) | lsw;
  }

  return adns_s_ok;
}

static void mf_soa(adns_query qu, void *datap) {
  adns_rr_soa *rrp= datap;

  adns__makefinal_str(qu,&rrp->mname);
  adns__makefinal_str(qu,&rrp->rname);
}

static adns_status cs_soa(vbuf *vb, const void *datap) {
  const adns_rr_soa *rrp= datap;
  char buf[20];
  int i;
  adns_status st;
  
  st= csp_domain(vb,rrp->mname);  if (st) return st;
  CSP_ADDSTR(" ");
  st= csp_mailbox(vb,rrp->rname);  if (st) return st;

  for (i=0; i<5; i++) {
    sprintf(buf," %lu",(&rrp->serial)[i]);
    CSP_ADDSTR(buf);
  }

  return adns_s_ok;
}

/*
 * _flat   (mf)
 */

static void mf_flat(adns_query qu, void *data) { }

/*
 * Now the table.
 */

#define TYPESZ_M(member)           (sizeof(*((adns_answer*)0)->rrs.member))

#define DEEP_MEMB(memb) TYPESZ_M(memb), mf_##memb, cs_##memb
#define FLAT_MEMB(memb) TYPESZ_M(memb), mf_flat, cs_##memb

#define DEEP_TYPE(code,rrt,fmt,memb,parser,comparer,printer) \
 { adns_r_##code, rrt, fmt, TYPESZ_M(memb), mf_##memb, printer, parser, comparer }
#define FLAT_TYPE(code,rrt,fmt,memb,parser,comparer,printer) \
 { adns_r_##code, rrt, fmt, TYPESZ_M(memb), mf_flat, printer, parser, comparer }

static const typeinfo typeinfos[] = {
/* Must be in ascending order of rrtype ! */
/* mem-mgmt code  rrt     fmt     member      parser      comparer    printer       */
  		    		     		 	     		 	       
FLAT_TYPE(a,      "A",     0,     inaddr,     pa_inaddr,  di_inaddr,  cs_inaddr     ),
DEEP_TYPE(ns_raw, "NS",   "raw",  str,        pa_host_raw,0,          cs_domain     ),
DEEP_TYPE(cname,  "CNAME", 0,     str,        pa_dom_raw, 0,          cs_domain     ),
DEEP_TYPE(soa_raw,"SOA",  "raw",  soa,        pa_soa,     0,          cs_soa        ),
DEEP_TYPE(ptr_raw,"PTR",  "raw",  str,        pa_host_raw,0,          cs_domain     ),
DEEP_TYPE(hinfo,  "HINFO", 0,     intstrpair, pa_hinfo,   0,          cs_hinfo      ),
DEEP_TYPE(mx_raw, "MX",   "raw",  intstr,     pa_mx_raw,  di_mx_raw,  cs_inthost    ),
DEEP_TYPE(txt,    "TXT",   0,     manyistr,   pa_txt,     0,          cs_txt        ),
DEEP_TYPE(rp_raw, "RP",   "raw",  strpair,    pa_rp,      0,          cs_rp         ),
 		     	                                     		   	       
FLAT_TYPE(addr,   "A",  "addr",   addr,       pa_addr,    di_addr,    cs_addr       ),
DEEP_TYPE(ns,     "NS", "+addr",  hostaddr,   pa_hostaddr,di_hostaddr,cs_hostaddr   ),
DEEP_TYPE(ptr,    "PTR","checked",str,        pa_ptr,     0,          cs_domain     ),
DEEP_TYPE(mx,     "MX", "+addr",  inthostaddr,pa_mx,      di_mx,      cs_inthostaddr),
 		     	                                     		      
DEEP_TYPE(soa,    "SOA","822",    soa,        pa_soa,     0,          cs_soa        ),
DEEP_TYPE(rp,     "RP", "822",    strpair,    pa_rp,      0,          cs_rp         ),
};

const typeinfo *adns__findtype(adns_rrtype type) {
  const typeinfo *begin, *end, *mid;

  begin= typeinfos;  end= typeinfos+(sizeof(typeinfos)/sizeof(typeinfo));

  while (begin < end) {
    mid= begin + ((end-begin)>>1);
    if (mid->type == type) return mid;
    if (type > mid->type) begin= mid+1;
    else end= mid;
  }
  return 0;
}
