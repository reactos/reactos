/*
 * poll.c
 * - wrappers for poll(2)
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

#include <limits.h>
#include <string.h>

#include "internal.h"

#ifdef HAVE_POLL

int adns_beforepoll(adns_state ads, struct pollfd *fds, int *nfds_io, int *timeout_io,
		    const struct timeval *now) {
  struct timeval tv_nowbuf, tv_tobuf, *tv_to;
  int space, found, timeout_ms, r;
  struct pollfd fds_tmp[MAX_POLLFDS];

  adns__consistency(ads,0,cc_entex);

  if (timeout_io) {
    adns__must_gettimeofday(ads,&now,&tv_nowbuf);
    if (!now) { *nfds_io= 0; r= 0; goto xit; }

    timeout_ms= *timeout_io;
    if (timeout_ms == -1) {
      tv_to= 0;
    } else {
      tv_tobuf.tv_sec= timeout_ms / 1000;
      tv_tobuf.tv_usec= (timeout_ms % 1000)*1000;
      tv_to= &tv_tobuf;
    }

    adns__timeouts(ads, 0, &tv_to,&tv_tobuf, *now);

    if (tv_to) {
      assert(tv_to == &tv_tobuf);
      timeout_ms= (tv_tobuf.tv_usec+999)/1000;
      assert(tv_tobuf.tv_sec < (INT_MAX-timeout_ms)/1000);
      timeout_ms += tv_tobuf.tv_sec*1000;
    } else {
      timeout_ms= -1;
    }
    *timeout_io= timeout_ms;
  }

  space= *nfds_io;
  if (space >= MAX_POLLFDS) {
    found= adns__pollfds(ads,fds);
    *nfds_io= found;
  } else {
    found= adns__pollfds(ads,fds_tmp);
    *nfds_io= found;
    if (space < found) { r= ERANGE; goto xit; }
    memcpy(fds,fds_tmp,sizeof(struct pollfd)*found);
  }
  r= 0;
xit:
  adns__consistency(ads,0,cc_entex);
  return r;
}

void adns_afterpoll(adns_state ads, const struct pollfd *fds, int nfds,
		    const struct timeval *now) {
  struct timeval tv_buf;

  adns__consistency(ads,0,cc_entex);
  adns__must_gettimeofday(ads,&now,&tv_buf);
  if (now) {
    adns__timeouts(ads, 1, 0,0, *now);
    adns__fdevents(ads, fds,nfds, 0,0,0,0, *now,0);
  }
  adns__consistency(ads,0,cc_entex);
}

int adns_wait_poll(adns_state ads,
		   adns_query *query_io,
		   adns_answer **answer_r,
		   void **context_r) {
  int r, nfds, to;
  struct pollfd fds[MAX_POLLFDS];

  adns__consistency(ads,0,cc_entex);

  for (;;) {
    r= adns__internal_check(ads,query_io,answer_r,context_r);
    if (r != EAGAIN) goto xit;
    nfds= MAX_POLLFDS; to= -1;
    adns_beforepoll(ads,fds,&nfds,&to,0);
    r= poll(fds,nfds,to);
    if (r == -1) {
      if (errno == EINTR) {
	if (ads->iflags & adns_if_eintr) { r= EINTR; goto xit; }
      } else {
	adns__diag(ads,-1,0,"poll failed in wait: %s",strerror(errno));
	adns_globalsystemfailure(ads);
      }
    } else {
      assert(r >= 0);
      adns_afterpoll(ads,fds,nfds,0);
    }
  }

 xit:
  adns__consistency(ads,0,cc_entex);
  return r;
}

#endif
