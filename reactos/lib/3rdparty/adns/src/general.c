/*
 * general.c
 * - diagnostic functions
 * - vbuf handling
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

#include <stdlib.h>

#ifdef ADNS_JGAA_WIN32
# include "adns_win32.h"
#else
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

#include "internal.h"

/* Core diagnostic functions */

void adns__vdiag(adns_state ads, const char *pfx, adns_initflags prevent,
		 int serv, adns_query qu, const char *fmt, va_list al) {
  const char *bef, *aft;
  vbuf vb;
  
  if (!ads->diagfile ||
      (!(ads->iflags & adns_if_debug) && (!prevent || (ads->iflags & prevent))))
    return;

  if (ads->iflags & adns_if_logpid) {
    fprintf(ads->diagfile,"adns%s [%ld]: ",pfx,(long)getpid());
  } else {
    fprintf(ads->diagfile,"adns%s: ",pfx);
  }

  vfprintf(ads->diagfile,fmt,al);

  bef= " (";
  aft= "\n";

  if (qu && qu->query_dgram) {
    adns__vbuf_init(&vb);
    fprintf(ads->diagfile,"%sQNAME=%s, QTYPE=%s",
	    bef,
	    adns__diag_domain(qu->ads,-1,0, &vb,
			      qu->query_dgram,qu->query_dglen,DNS_HDRSIZE),
	    qu->typei ? qu->typei->rrtname : "<unknown>");
    if (qu->typei && qu->typei->fmtname)
      fprintf(ads->diagfile,"(%s)",qu->typei->fmtname);
    bef=", "; aft=")\n";
    adns__vbuf_free(&vb);
  }
  
  if (serv>=0) {
    fprintf(ads->diagfile,"%sNS=%s",bef,inet_ntoa(ads->servers[serv].addr));
    bef=", "; aft=")\n";
  }

  fputs(aft,ads->diagfile);
}

void adns__debug(adns_state ads, int serv, adns_query qu, const char *fmt, ...) {
  va_list al;

  va_start(al,fmt);
  adns__vdiag(ads," debug",0,serv,qu,fmt,al);
  va_end(al);
}

void adns__warn(adns_state ads, int serv, adns_query qu, const char *fmt, ...) {
  va_list al;

  va_start(al,fmt);
  adns__vdiag(ads," warning",adns_if_noerrprint|adns_if_noserverwarn,serv,qu,fmt,al);
  va_end(al);
}

void adns__diag(adns_state ads, int serv, adns_query qu, const char *fmt, ...) {
  va_list al;

  va_start(al,fmt);
  adns__vdiag(ads,"",adns_if_noerrprint,serv,qu,fmt,al);
  va_end(al);
}

/* vbuf functions */

void adns__vbuf_init(vbuf *vb) {
  vb->used= vb->avail= 0; vb->buf= 0;
}

int adns__vbuf_ensure(vbuf *vb, int want) {
  void *nb;
  
  if (vb->avail >= want) return 1;
  nb= realloc(vb->buf, (size_t) want); if (!nb) return 0;
  vb->buf= nb;
  vb->avail= want;
  return 1;
}
  
void adns__vbuf_appendq(vbuf *vb, const byte *data, int len) {
  memcpy(vb->buf+vb->used,data, (size_t) len);
  vb->used+= len;
}

int adns__vbuf_append(vbuf *vb, const byte *data, int len) {
  int newlen;
  void *nb;

  newlen= vb->used+len;
  if (vb->avail < newlen) {
    if (newlen<20) newlen= 20;
    newlen <<= 1;
    nb= realloc(vb->buf,(size_t) newlen);
    if (!nb) { newlen= vb->used+len; nb= realloc(vb->buf, (size_t) newlen); }
    if (!nb) return 0;
    vb->buf= nb;
    vb->avail= newlen;
  }
  adns__vbuf_appendq(vb,data,len);
  return 1;
}

int adns__vbuf_appendstr(vbuf *vb, const char *data) {
  int l;
  l= strlen(data);
  return adns__vbuf_append(vb,(byte*)data,l);
}

void adns__vbuf_free(vbuf *vb) {
  free(vb->buf);
  adns__vbuf_init(vb);
}

/* Additional diagnostic functions */

const char *adns__diag_domain(adns_state ads, int serv, adns_query qu,
			      vbuf *vb, const byte *dgram, int dglen, int cbyte) {
  adns_status st;

  st= adns__parse_domain(ads,serv,qu,vb, pdf_quoteok, dgram,dglen,&cbyte,dglen);
  if (st == adns_s_nomemory) {
    return "<cannot report domain... out of memory>";
  }
  if (st) {
    vb->used= 0;
    if (!(adns__vbuf_appendstr(vb,"<bad format... ") &&
	  adns__vbuf_appendstr(vb,adns_strerror(st)) &&
	  adns__vbuf_appendstr(vb,">") &&
	  adns__vbuf_append(vb,(byte*)"",1))) {
      return "<cannot report bad format... out of memory>";
    }
  }
  if (!vb->used) {
    adns__vbuf_appendstr(vb,"<truncated ...>");
    adns__vbuf_append(vb,(byte*)"",1);
  }
  return (char*)vb->buf;
}

adns_status adns_rr_info(adns_rrtype type,
			 const char **rrtname_r, const char **fmtname_r,
			 int *len_r,
			 const void *datap, char **data_r) {
  const typeinfo *typei;
  vbuf vb;
  adns_status st;

  typei= adns__findtype(type);
  if (!typei) return adns_s_unknownrrtype;

  if (rrtname_r) *rrtname_r= typei->rrtname;
  if (fmtname_r) *fmtname_r= typei->fmtname;
  if (len_r) *len_r= typei->rrsz;

  if (!datap) return adns_s_ok;
  
  adns__vbuf_init(&vb);
  st= typei->convstring(&vb,datap);
  if (st) goto x_freevb;
  if (!adns__vbuf_append(&vb,(byte*)"",1)) { st= adns_s_nomemory; goto x_freevb; }
  assert((int)strlen((char*)vb.buf) == vb.used-1);
  *data_r= realloc(vb.buf, (size_t) vb.used);
  if (!*data_r) *data_r= (char*)vb.buf;
  return adns_s_ok;

 x_freevb:
  adns__vbuf_free(&vb);
  return st;
}


#define SINFO(n,s) { adns_s_##n, #n, s }

static const struct sinfo {
  adns_status st;
  const char *abbrev;
  const char *string;
} sinfos[]= {
  SINFO(  ok,                  "OK"                                            ),

  SINFO(  nomemory,            "Out of memory"                                 ),
  SINFO(  unknownrrtype,       "Query not implemented in DNS library"          ),
  SINFO(  systemfail,          "General resolver or system failure"            ),

  SINFO(  timeout,             "DNS query timed out"                           ),
  SINFO(  allservfail,         "All nameservers failed"                        ),
  SINFO(  norecurse,           "Recursion denied by nameserver"                ),
  SINFO(  invalidresponse,     "Nameserver sent bad response"                  ),
  SINFO(  unknownformat,       "Nameserver used unknown format"                ),

  SINFO(  rcodeservfail,       "Nameserver reports failure"                    ),
  SINFO(  rcodeformaterror,    "Query not understood by nameserver"            ),
  SINFO(  rcodenotimplemented, "Query not implemented by nameserver"           ),
  SINFO(  rcoderefused,        "Query refused by nameserver"                   ),
  SINFO(  rcodeunknown,        "Nameserver sent unknown response code"         ),
  
  SINFO(  inconsistent,        "Inconsistent resource records in DNS"          ),
  SINFO(  prohibitedcname,     "DNS alias found where canonical name wanted"   ),
  SINFO(  answerdomaininvalid, "Found syntactically invalid domain name"       ),
  SINFO(  answerdomaintoolong, "Found overly-long domain name"                 ),
  SINFO(  invaliddata,         "Found invalid DNS data"                        ),

  SINFO(  querydomainwrong,    "Domain invalid for particular DNS query type"  ),
  SINFO(  querydomaininvalid,  "Domain name is syntactically invalid"          ),
  SINFO(  querydomaintoolong,  "Domain name or component is too long"          ),

  SINFO(  nxdomain,            "No such domain"                                ),
  SINFO(  nodata,              "No such data"                                  )
};

static int __cdecl si_compar(const void *key, const void *elem) {
  const adns_status *st= key;
  const struct sinfo *si= elem;

  return *st < si->st ? -1 : *st > si->st ? 1 : 0;
}

static const struct sinfo *findsinfo(adns_status st) {
  return bsearch(&st,sinfos,sizeof(sinfos)/sizeof(*sinfos),sizeof(*sinfos),si_compar);
}

const char *adns_strerror(adns_status st) {
  const struct sinfo *si;

  si= findsinfo(st);
  return si->string;
}

const char *adns_errabbrev(adns_status st) {
  const struct sinfo *si;

  si= findsinfo(st);
  return si->abbrev;
}


#define STINFO(max) { adns_s_max_##max, #max }

static const struct stinfo {
  adns_status stmax;
  const char *abbrev;
} stinfos[]= {
  { adns_s_ok, "ok" },
  STINFO(  localfail   ),
  STINFO(  remotefail  ),
  STINFO(  tempfail    ),
  STINFO(  misconfig   ),
  STINFO(  misquery    ),
  STINFO(  permfail    )
};

static int __cdecl sti_compar(const void *key, const void *elem) {
  const adns_status *st= key;
  const struct stinfo *sti= elem;

  adns_status here, min, max;

  here= *st;
  min= (sti==stinfos) ? 0 : sti[-1].stmax+1;
  max= sti->stmax;
  
  return here < min  ? -1 : here > max ? 1 : 0;
}

const char *adns_errtypeabbrev(adns_status st) {
  const struct stinfo *sti;

  sti= bsearch(&st,stinfos,sizeof(stinfos)/sizeof(*stinfos),sizeof(*stinfos),sti_compar);
  return sti->abbrev;
}


void adns__isort(void *array, int nobjs, int sz, void *tempbuf,
		 int (*needswap)(void *context, const void *a, const void *b),
		 void *context) {
  byte *data= array;
  int i, place;

  for (i=0; i<nobjs; i++) {
    for (place= i;
	 place>0 && needswap(context, data + (place-1)*sz, data + i*sz);
	 place--);
    if (place != i) {
      memcpy(tempbuf, data + i*sz, (size_t) sz);
      memmove(data + (place+1)*sz, data + place*sz, (size_t) (i-place)*sz);
      memcpy(data + place*sz, tempbuf, (size_t) sz);
    }
  }
}

/* SIGPIPE protection. */
/* Not required under Win32 with MSVC */

void adns__sigpipe_protect(adns_state ads) {
#ifndef ADNS_JGAA_WIN32
  sigset_t toblock;
  struct sigaction sa;
  int r;

  if (ads->iflags & adns_if_nosigpipe) return;

  sigfillset(&toblock);
  sigdelset(&toblock,SIGPIPE);

  sa.sa_handler= SIG_IGN;
  sigfillset(&sa.sa_mask);
  sa.sa_flags= 0;
  
  r= sigprocmask(SIG_SETMASK,&toblock,&ads->stdsigmask); assert(!r);
  r= sigaction(SIGPIPE,&sa,&ads->stdsigpipe); assert(!r);
#endif
}

void adns__sigpipe_unprotect(adns_state ads) {
#ifndef ADNS_JGAA_WIN32
  int r;

  if (ads->iflags & adns_if_nosigpipe) return;

  r= sigaction(SIGPIPE,&ads->stdsigpipe,0); assert(!r);
  r= sigprocmask(SIG_SETMASK,&ads->stdsigmask,0); assert(!r);
#endif
}
