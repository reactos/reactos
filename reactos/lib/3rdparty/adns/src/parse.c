/*
 * parse.c
 * - parsing assistance functions (mainly for domains inside datagrams)
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

#include "internal.h"

int vbuf__append_quoted1035(vbuf *vb, const byte *buf, int len) {
  char qbuf[10];
  int i, ch;

  while (len) {
    qbuf[0]= 0;
    for (i=0; i<len; i++) {
      ch= buf[i];
      if (ch <= ' ' || ch >= 127) {
	sprintf(qbuf,"\\%03o",ch);
	break;
      } else if (!ctype_domainunquoted(ch)) {
	sprintf(qbuf,"\\%c",ch);
	break;
      }
    }
    if (!adns__vbuf_append(vb,buf,i) || !adns__vbuf_append(vb,(byte*)qbuf,(int)  strlen(qbuf)))
      return 0;
    if (i<len) i++;
    buf+= i;
    len-= i;
  }
  return 1;
}

void adns__findlabel_start(findlabel_state *fls, adns_state ads,
			   int serv, adns_query qu,
			   const byte *dgram, int dglen, int max,
			   int dmbegin, int *dmend_rlater) {
  fls->ads= ads;
  fls->qu= qu;
  fls->serv= serv;
  fls->dgram= dgram;
  fls->dglen= dglen;
  fls->max= max;
  fls->cbyte= dmbegin;
  fls->namelen= 0;
  fls->dmend_r= dmend_rlater;
}

adns_status adns__findlabel_next(findlabel_state *fls,
				 int *lablen_r, int *labstart_r) {
  int lablen, jumpto;
  const char *dgram;

  dgram= (char*)fls->dgram;
  for (;;) {
    if (fls->cbyte >= fls->dglen) goto x_truncated;
    if (fls->cbyte >= fls->max) goto x_badresponse;
    GET_B(fls->cbyte,lablen);
    if (!(lablen & 0x0c0)) break;
    if ((lablen & 0x0c0) != 0x0c0) return adns_s_unknownformat;
    if (fls->cbyte >= fls->dglen) goto x_truncated;
    if (fls->cbyte >= fls->max) goto x_badresponse;
    GET_B(fls->cbyte,jumpto);
    jumpto |= (lablen&0x3f)<<8;
    if (fls->dmend_r) *(fls->dmend_r)= fls->cbyte;
    fls->cbyte= jumpto;
    fls->dmend_r= 0; fls->max= fls->dglen+1;
  }
  if (labstart_r) *labstart_r= fls->cbyte;
  if (lablen) {
    if (fls->namelen) fls->namelen++;
    fls->namelen+= lablen;
    if (fls->namelen > DNS_MAXDOMAIN) return adns_s_answerdomaintoolong;
    fls->cbyte+= lablen;
    if (fls->cbyte > fls->dglen) goto x_truncated;
    if (fls->cbyte > fls->max) goto x_badresponse;
  } else {
    if (fls->dmend_r) *(fls->dmend_r)= fls->cbyte;
  }
  *lablen_r= lablen;
  return adns_s_ok;

 x_truncated:
  *lablen_r= -1;
  return adns_s_ok;

 x_badresponse:
  adns__diag(fls->ads,fls->serv,fls->qu,"label in domain runs beyond end of domain");
  return adns_s_invalidresponse;
}

adns_status adns__parse_domain(adns_state ads, int serv, adns_query qu,
			       vbuf *vb, parsedomain_flags flags,
			       const byte *dgram, int dglen, int *cbyte_io, int max) {
  findlabel_state fls;

  adns__findlabel_start(&fls,ads, serv,qu, dgram,dglen,max, *cbyte_io,cbyte_io);
  vb->used= 0;
  return adns__parse_domain_more(&fls,ads,qu, vb,flags,dgram);
}

adns_status adns__parse_domain_more(findlabel_state *fls, adns_state ads,
				    adns_query qu, vbuf *vb, parsedomain_flags flags,
				    const byte *dgram) {
  int lablen, labstart, i, ch, first;
  adns_status st;

  first= 1;
  for (;;) {
    st= adns__findlabel_next(fls,&lablen,&labstart);
    if (st) return st;
    if (lablen<0) { vb->used=0; return adns_s_ok; }
    if (!lablen) break;
    if (first) {
      first= 0;
    } else {
      if (!adns__vbuf_append(vb,(byte*)".",1)) return adns_s_nomemory;
    }
    if (flags & pdf_quoteok) {
      if (!vbuf__append_quoted1035(vb,dgram+labstart,lablen))
	return adns_s_nomemory;
    } else {
      ch= dgram[labstart];
      if (!ctype_alpha(ch) && !ctype_digit(ch)) return adns_s_answerdomaininvalid;
      for (i= labstart+1; i<labstart+lablen; i++) {
	ch= dgram[i];
	if (ch != '-' && !ctype_alpha(ch) && !ctype_digit(ch))
	  return adns_s_answerdomaininvalid;
      }
      if (!adns__vbuf_append(vb,dgram+labstart,lablen))
	return adns_s_nomemory;
    }
  }
  if (!adns__vbuf_append(vb,(byte*)"",1)) return adns_s_nomemory;
  return adns_s_ok;
}

adns_status adns__findrr_anychk(adns_query qu, int serv,
				const byte *dgram, int dglen, int *cbyte_io,
				int *type_r, int *class_r, unsigned long *ttl_r,
				int *rdlen_r, int *rdstart_r,
				const byte *eo_dgram, int eo_dglen, int eo_cbyte,
				int *eo_matched_r) {
  findlabel_state fls, eo_fls;
  int cbyte;

  int tmp, rdlen, mismatch;
  unsigned long ttl;
  int lablen, labstart, ch;
  int eo_lablen, eo_labstart, eo_ch;
  adns_status st;

  cbyte= *cbyte_io;

  adns__findlabel_start(&fls,qu->ads, serv,qu, dgram,dglen,dglen,cbyte,&cbyte);
  if (eo_dgram) {
    adns__findlabel_start(&eo_fls,qu->ads, -1,0, eo_dgram,eo_dglen,eo_dglen,eo_cbyte,0);
    mismatch= 0;
  } else {
    mismatch= 1;
  }

  for (;;) {
    st= adns__findlabel_next(&fls,&lablen,&labstart);
    if (st) return st;
    if (lablen<0) goto x_truncated;

    if (!mismatch) {
      st= adns__findlabel_next(&eo_fls,&eo_lablen,&eo_labstart);
      assert(!st); assert(eo_lablen>=0);
      if (lablen != eo_lablen) mismatch= 1;
      while (!mismatch && eo_lablen-- > 0) {
	ch= dgram[labstart++]; if (ctype_alpha(ch)) ch &= ~32;
	eo_ch= eo_dgram[eo_labstart++]; if (ctype_alpha(eo_ch)) eo_ch &= ~32;
	if (ch != eo_ch) mismatch= 1;
      }
    }
    if (!lablen) break;
  }
  if (eo_matched_r) *eo_matched_r= !mismatch;

  if (cbyte+10>dglen) goto x_truncated;
  GET_W(cbyte,tmp); *type_r= tmp;
  GET_W(cbyte,tmp); *class_r= tmp;

  GET_L(cbyte,ttl);
  if (ttl > MAXTTLBELIEVE) ttl= MAXTTLBELIEVE;
  *ttl_r= ttl;

  GET_W(cbyte,rdlen); if (rdlen_r) *rdlen_r= rdlen;
  if (rdstart_r) *rdstart_r= cbyte;
  cbyte+= rdlen;
  if (cbyte>dglen) goto x_truncated;
  *cbyte_io= cbyte;
  return adns_s_ok;

 x_truncated:
  *type_r= -1;
  return 0;
}

adns_status adns__findrr(adns_query qu, int serv,
			 const byte *dgram, int dglen, int *cbyte_io,
			 int *type_r, int *class_r, unsigned long *ttl_r,
			 int *rdlen_r, int *rdstart_r,
			 int *ownermatchedquery_r) {
  if (!ownermatchedquery_r) {
    return adns__findrr_anychk(qu,serv,
			       dgram,dglen,cbyte_io,
			       type_r,class_r,ttl_r,rdlen_r,rdstart_r,
			       0,0,0, 0);
  } else if (!qu->cname_dgram) {
    return adns__findrr_anychk(qu,serv,
			       dgram,dglen,cbyte_io,
			       type_r,class_r,ttl_r,rdlen_r,rdstart_r,
			       qu->query_dgram,qu->query_dglen,DNS_HDRSIZE,
			       ownermatchedquery_r);
  } else {
    return adns__findrr_anychk(qu,serv,
			       dgram,dglen,cbyte_io,
			       type_r,class_r,ttl_r,rdlen_r,rdstart_r,
			       qu->cname_dgram,qu->cname_dglen,qu->cname_begin,
			       ownermatchedquery_r);
  }
}
