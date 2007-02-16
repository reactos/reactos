/*
 * event.c
 * - event loop core
 * - TCP connection management
 * - user-visible check/wait and event-loop-related functions
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
 *  along with this program; if not, adns_socket_write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#include <errno.h>
# include "adns_win32.h"
#include <stdlib.h>

#ifdef ADNS_JGAA_WIN32
#else
# include <unistd.h>
# include <sys/types.h>
# include <sys/time.h>
# include <netdb.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

#include "internal.h"
#include "tvarith.h"

/* TCP connection management. */

static void tcp_close(adns_state ads) {
  int serv;
  
  serv= ads->tcpserver;
  adns_socket_close(ads->tcpsocket);
  ads->tcpsocket= INVALID_SOCKET;
  ads->tcprecv.used= ads->tcprecv_skip= ads->tcpsend.used= 0;
}

void adns__tcp_broken(adns_state ads, const char *what, const char *why) {
  int serv;
  adns_query qu;
  
  assert(ads->tcpstate == server_connecting || ads->tcpstate == server_ok);
  serv= ads->tcpserver;
  if (what) adns__warn(ads,serv,0,"TCP connection failed: %s: %s",what,why);

  if (ads->tcpstate == server_connecting) {
    /* Counts as a retry for all the queries waiting for TCP. */
    for (qu= ads->tcpw.head; qu; qu= qu->next)
      qu->retries++;
  }

  tcp_close(ads);
  ads->tcpstate= server_broken;
  ads->tcpserver= (serv+1)%ads->nservers;
}

static void tcp_connected(adns_state ads, struct timeval now) {
  adns_query qu, nqu;
  
  adns__debug(ads,ads->tcpserver,0,"TCP connected");
  ads->tcpstate= server_ok;
  for (qu= ads->tcpw.head; qu && ads->tcpstate == server_ok; qu= nqu) {
    nqu= qu->next;
    assert(qu->state == query_tcpw);
    adns__querysend_tcp(qu,now);
  }
}

void adns__tcp_tryconnect(adns_state ads, struct timeval now) {
  int r, tries;
  ADNS_SOCKET fd;
  struct sockaddr_in addr;
  struct protoent *proto;

  for (tries=0; tries<ads->nservers; tries++) {
    switch (ads->tcpstate) {
    case server_connecting:
    case server_ok:
    case server_broken:
      return;
    case server_disconnected:
      break;
    default:
      abort();
    }
    
    assert(!ads->tcpsend.used);
    assert(!ads->tcprecv.used);
    assert(!ads->tcprecv_skip);

    proto= getprotobyname("tcp");
    if (!proto) { adns__diag(ads,-1,0,"unable to find protocol no. for TCP !"); return; }
	ADNS_CLEAR_ERRNO
    fd= socket(AF_INET,SOCK_STREAM,proto->p_proto);
	ADNS_CAPTURE_ERRNO;
    if (fd == INVALID_SOCKET) {
      adns__diag(ads,-1,0,"cannot create TCP socket: %s",strerror(errno));
      return;
    }
    r= adns__setnonblock(ads,fd);
    if (r) {
      adns__diag(ads,-1,0,"cannot make TCP socket nonblocking: %s",strerror(r));
      adns_socket_close(fd);
      return;
    }
    memset(&addr,0,sizeof(addr));
    addr.sin_family= AF_INET;
    addr.sin_port= htons(DNS_PORT);
    addr.sin_addr= ads->servers[ads->tcpserver].addr;
    r= connect(fd,(const struct sockaddr*)&addr,sizeof(addr));
    ads->tcpsocket= fd;
    ads->tcpstate= server_connecting;
    if (r==0) { tcp_connected(ads,now); return; }
    if (errno == EWOULDBLOCK || errno == EINPROGRESS) {
      ads->tcptimeout= now;
      timevaladd(&ads->tcptimeout,TCPCONNMS);
      return;
    }
    adns__tcp_broken(ads,"connect",strerror(errno));
    ads->tcpstate= server_disconnected;
  }
}

/* Timeout handling functions. */

void adns__must_gettimeofday(adns_state ads, const struct timeval **now_io,
			     struct timeval *tv_buf) {
  const struct timeval *now;
  int r;

  now= *now_io;
  if (now) return;
  r= gettimeofday(tv_buf,0); if (!r) { *now_io= tv_buf; return; }
  adns__diag(ads,-1,0,"gettimeofday failed: %s",strerror(errno));
  adns_globalsystemfailure(ads);
  return;
}

static void inter_immed(struct timeval **tv_io, struct timeval *tvbuf) {
  struct timeval *rbuf;

  if (!tv_io) return;

  rbuf= *tv_io;
  if (!rbuf) { *tv_io= rbuf= tvbuf; }

  timerclear(rbuf);
}
    
static void inter_maxto(struct timeval **tv_io, struct timeval *tvbuf,
			struct timeval maxto) {
  struct timeval *rbuf;

  if (!tv_io) return;
  rbuf= *tv_io;
  if (!rbuf) {
    *tvbuf= maxto; *tv_io= tvbuf;
  } else {
    if (timercmp(rbuf,&maxto,>)) *rbuf= maxto;
  }
/*fprintf(stderr,"inter_maxto maxto=%ld.%06ld result=%ld.%06ld\n",
	maxto.tv_sec,maxto.tv_usec,(**tv_io).tv_sec,(**tv_io).tv_usec);*/
}

static void inter_maxtoabs(struct timeval **tv_io, struct timeval *tvbuf,
			   struct timeval now, struct timeval maxtime) {
  /* tv_io may be 0 */
  ldiv_t dr;

/*fprintf(stderr,"inter_maxtoabs now=%ld.%06ld maxtime=%ld.%06ld\n",
	now.tv_sec,now.tv_usec,maxtime.tv_sec,maxtime.tv_usec);*/
  if (!tv_io) return;
  maxtime.tv_sec -= (now.tv_sec+2);
  maxtime.tv_usec -= (now.tv_usec-2000000);
  dr= ldiv(maxtime.tv_usec,1000000);
  maxtime.tv_sec += dr.quot;
  maxtime.tv_usec -= dr.quot*1000000;
  if (maxtime.tv_sec<0) timerclear(&maxtime);
  inter_maxto(tv_io,tvbuf,maxtime);
}

static void timeouts_queue(adns_state ads, int act,
			   struct timeval **tv_io, struct timeval *tvbuf,
			   struct timeval now, struct query_queue *queue) {
  adns_query qu, nqu;
  
  for (qu= queue->head; qu; qu= nqu) {
    nqu= qu->next;
    if (!timercmp(&now,&qu->timeout,>)) {
      inter_maxtoabs(tv_io,tvbuf,now,qu->timeout);
    } else {
      if (!act) { inter_immed(tv_io,tvbuf); return; }
      LIST_UNLINK(*queue,qu);
      if (qu->state != query_tosend) {
	adns__query_fail(qu,adns_s_timeout);
      } else {
	adns__query_send(qu,now);
      }
      nqu= queue->head;
    }
  }
}

static void tcp_events(adns_state ads, int act,
		       struct timeval **tv_io, struct timeval *tvbuf,
		       struct timeval now) {
  adns_query qu, nqu;
  
  for (;;) {
    switch (ads->tcpstate) {
    case server_broken:
      if (!act) { inter_immed(tv_io,tvbuf); return; }
      for (qu= ads->tcpw.head; qu; qu= nqu) {
	nqu= qu->next;
	assert(qu->state == query_tcpw);
	if (qu->retries > ads->nservers) {
	  LIST_UNLINK(ads->tcpw,qu);
	  adns__query_fail(qu,adns_s_allservfail);
	}
      }
      ads->tcpstate= server_disconnected;
    case server_disconnected: /* fall through */
      if (!ads->tcpw.head) return;
      if (!act) { inter_immed(tv_io,tvbuf); return; }
      adns__tcp_tryconnect(ads,now);
      break;
    case server_ok:
      if (ads->tcpw.head) return;
      if (!ads->tcptimeout.tv_sec) {
	assert(!ads->tcptimeout.tv_usec);
	ads->tcptimeout= now;
	timevaladd(&ads->tcptimeout,TCPIDLEMS);
      }
    case server_connecting: /* fall through */
      if (!act || !timercmp(&now,&ads->tcptimeout,>)) {
	inter_maxtoabs(tv_io,tvbuf,now,ads->tcptimeout);
	return;
      } {
	/* TCP timeout has happened */
	switch (ads->tcpstate) {
	case server_connecting: /* failed to connect */
	  adns__tcp_broken(ads,"unable to make connection","timed out");
	  break;
	case server_ok: /* idle timeout */
	  tcp_close(ads);
	  ads->tcpstate= server_disconnected;
	  return;
	default:
	  abort();
	}
      }
      break;
    default:
      abort();
    }
  }
  return;
}

void adns__timeouts(adns_state ads, int act,
		    struct timeval **tv_io, struct timeval *tvbuf,
		    struct timeval now) {
  timeouts_queue(ads,act,tv_io,tvbuf,now, &ads->udpw);
  timeouts_queue(ads,act,tv_io,tvbuf,now, &ads->tcpw);
  tcp_events(ads,act,tv_io,tvbuf,now);
}

void adns_firsttimeout(adns_state ads,
		       struct timeval **tv_io, struct timeval *tvbuf,
		       struct timeval now) {
  adns__consistency(ads,0,cc_entex);
  adns__timeouts(ads, 0, tv_io,tvbuf, now);
  adns__consistency(ads,0,cc_entex);
}

void adns_processtimeouts(adns_state ads, const struct timeval *now) {
  struct timeval tv_buf;

  adns__consistency(ads,0,cc_entex);
  adns__must_gettimeofday(ads,&now,&tv_buf);
  if (now) adns__timeouts(ads, 1, 0,0, *now);
  adns__consistency(ads,0,cc_entex);
}

/* fd handling functions.  These are the top-level of the real work of
 * reception and often transmission.
 */

int adns__pollfds(adns_state ads, struct pollfd pollfds_buf[MAX_POLLFDS]) {
  /* Returns the number of entries filled in.  Always zeroes revents. */

  assert(MAX_POLLFDS==2);

  pollfds_buf[0].fd= ads->udpsocket;
  pollfds_buf[0].events= POLLIN;
  pollfds_buf[0].revents= 0;

  switch (ads->tcpstate) {
  case server_disconnected:
  case server_broken:
    return 1;
  case server_connecting:
    pollfds_buf[1].events= POLLOUT;
    break;
  case server_ok:
    pollfds_buf[1].events= ads->tcpsend.used ? POLLIN|POLLOUT|POLLPRI : POLLIN|POLLPRI;
    break;
  default:
    abort();
  }
  pollfds_buf[1].fd= ads->tcpsocket;
  return 2;
}

int adns_processreadable(adns_state ads, ADNS_SOCKET fd, const struct timeval *now) {
  int want, dgramlen, r, udpaddrlen, serv, old_skip;
  byte udpbuf[DNS_MAXUDP];
  struct sockaddr_in udpaddr;
  
  adns__consistency(ads,0,cc_entex);

  switch (ads->tcpstate) {
  case server_disconnected:
  case server_broken:
  case server_connecting:
    break;
  case server_ok:
    if (fd != ads->tcpsocket) break;
    assert(!ads->tcprecv_skip);
    do {
      if (ads->tcprecv.used >= ads->tcprecv_skip+2) {
	dgramlen= ((ads->tcprecv.buf[ads->tcprecv_skip]<<8) |
	           ads->tcprecv.buf[ads->tcprecv_skip+1]);
	if (ads->tcprecv.used >= ads->tcprecv_skip+2+dgramlen) {
	  old_skip= ads->tcprecv_skip;
	  ads->tcprecv_skip += 2+dgramlen;
	  adns__procdgram(ads, ads->tcprecv.buf+old_skip+2,
			  dgramlen, ads->tcpserver, 1,*now);
	  continue;
	} else {
	  want= 2+dgramlen;
	}
      } else {
	want= 2;
      }
      ads->tcprecv.used -= ads->tcprecv_skip;
      memmove(ads->tcprecv.buf,ads->tcprecv.buf+ads->tcprecv_skip, (size_t) ads->tcprecv.used);
      ads->tcprecv_skip= 0;
      if (!adns__vbuf_ensure(&ads->tcprecv,want)) { r= ENOMEM; goto xit; }
      assert(ads->tcprecv.used <= ads->tcprecv.avail);
      if (ads->tcprecv.used == ads->tcprecv.avail) continue;
	  ADNS_CLEAR_ERRNO;
      r= adns_socket_read(ads->tcpsocket,
	      ads->tcprecv.buf+ads->tcprecv.used,
	      ads->tcprecv.avail-ads->tcprecv.used);
	  ADNS_CAPTURE_ERRNO;
      if (r>0) {
	ads->tcprecv.used+= r;
      } else {
	if (r) {
	  if (errno==EAGAIN || errno==EWOULDBLOCK) { r= 0; goto xit; }
	  if (errno==EINTR) continue;
	  if (errno_resources(errno)) { r= errno; goto xit; }
	}
	adns__tcp_broken(ads,"adns_socket_read",r?strerror(errno):"closed");
      }
    } while (ads->tcpstate == server_ok);
    r= 0; goto xit;
  default:
    abort();
  }
  if (fd == ads->udpsocket) {
    for (;;) {
      udpaddrlen= sizeof(udpaddr);
	  ADNS_CLEAR_ERRNO;
      r= recvfrom(ads->udpsocket,(char*)udpbuf,sizeof(udpbuf),0,
		  (struct sockaddr*)&udpaddr,&udpaddrlen);
	  ADNS_CAPTURE_ERRNO;
      if (r<0) {
	if (errno == EAGAIN || errno == EWOULDBLOCK) { r= 0; goto xit; }
	if (errno == EINTR) continue;
	if (errno_resources(errno)) { r= errno; goto xit; }
	adns__warn(ads,-1,0,"datagram receive error: %s",strerror(errno));
	r= 0; goto xit;
      }
      if (udpaddrlen != sizeof(udpaddr)) {
	adns__diag(ads,-1,0,"datagram received with wrong address length %d"
		   " (expected %lu)", udpaddrlen,
		   (unsigned long)sizeof(udpaddr));
	continue;
      }
      if (udpaddr.sin_family != AF_INET) {
	adns__diag(ads,-1,0,"datagram received with wrong protocol family"
		   " %u (expected %u)",udpaddr.sin_family,AF_INET);
	continue;
      }
      if (ntohs(udpaddr.sin_port) != DNS_PORT) {
	adns__diag(ads,-1,0,"datagram received from wrong port %u (expected %u)",
		   ntohs(udpaddr.sin_port),DNS_PORT);
	continue;
      }
      for (serv= 0;
	   serv < ads->nservers &&
	     ads->servers[serv].addr.s_addr != udpaddr.sin_addr.s_addr;
	   serv++);
      if (serv >= ads->nservers) {
	adns__warn(ads,-1,0,"datagram received from unknown nameserver %s",
		   inet_ntoa(udpaddr.sin_addr));
	continue;
      }
      adns__procdgram(ads,udpbuf,r,serv,0,*now);
    }
  }
  r= 0;
xit:
  adns__consistency(ads,0,cc_entex);
  return r;
}

int adns_processwriteable(adns_state ads, ADNS_SOCKET fd, const struct timeval *now) {
  int r;
  
  adns__consistency(ads,0,cc_entex);

  switch (ads->tcpstate) {
  case server_disconnected:
  case server_broken:
    break;
  case server_connecting:
    if (fd != ads->tcpsocket) break;
    assert(ads->tcprecv.used==0);
    assert(ads->tcprecv_skip==0);
    for (;;) {
      if (!adns__vbuf_ensure(&ads->tcprecv,1)) { r= ENOMEM; goto xit; }
	  ADNS_CLEAR_ERRNO;
      r= adns_socket_read(ads->tcpsocket,&ads->tcprecv.buf,1);
	  ADNS_CAPTURE_ERRNO;
      if (r==0 || (r<0 && (errno==EAGAIN || errno==EWOULDBLOCK))) {
	tcp_connected(ads,*now);
	r= 0; goto xit;
      }
      if (r>0) {
	adns__tcp_broken(ads,"connect/adns_socket_read","sent data before first request");
	r= 0; goto xit;
      }
      if (errno==EINTR) continue;
      if (errno_resources(errno)) { r= errno; goto xit; }
      adns__tcp_broken(ads,"connect/adns_socket_read",strerror(errno));
      r= 0; goto xit;
    } /* not reached */
  case server_ok:
    if (fd != ads->tcpsocket) break;
    while (ads->tcpsend.used) {
      adns__sigpipe_protect(ads);
	  ADNS_CLEAR_ERRNO;
      r= adns_socket_write(ads->tcpsocket,ads->tcpsend.buf,ads->tcpsend.used);
	  ADNS_CAPTURE_ERRNO;
      adns__sigpipe_unprotect(ads);
      if (r<0) {
	if (errno==EINTR) continue;
	if (errno==EAGAIN || errno==EWOULDBLOCK) { r= 0; goto xit; }
	if (errno_resources(errno)) { r= errno; goto xit; }
	adns__tcp_broken(ads,"adns_socket_write",strerror(errno));
	r= 0; goto xit;
      } else if (r>0) {
	ads->tcpsend.used -= r;
	memmove(ads->tcpsend.buf,ads->tcpsend.buf+r, (size_t) ads->tcpsend.used);
      }
    }
    r= 0;
    goto xit;
  default:
    abort();
  }
  r= 0;
xit:
  adns__consistency(ads,0,cc_entex);
  return r;
}
  
int adns_processexceptional(adns_state ads, ADNS_SOCKET fd, const struct timeval *now) {
  adns__consistency(ads,0,cc_entex);
  switch (ads->tcpstate) {
  case server_disconnected:
  case server_broken:
    break;
  case server_connecting:
  case server_ok:
    if (fd != ads->tcpsocket) break;
    adns__tcp_broken(ads,"poll/select","exceptional condition detected");
    break;
  default:
    abort();
  }
  adns__consistency(ads,0,cc_entex);
  return 0;
}

static void fd_event(adns_state ads, ADNS_SOCKET fd,
		     int revent, int pollflag,
		     int maxfd, const fd_set *fds,
		     int (*func)(adns_state, ADNS_SOCKET fd, const struct timeval *now),
		     struct timeval now, int *r_r) {
  int r;
  
  if (!(revent & pollflag)) return;
  if (fds && !((int)fd<maxfd && FD_ISSET(fd,fds))) return;
  r= func(ads,fd,&now);
  if (r) {
    if (r_r) {
      *r_r= r;
    } else {
      adns__diag(ads,-1,0,"process fd failed after select: %s",strerror(errno));
      adns_globalsystemfailure(ads);
    }
  }
}

void adns__fdevents(adns_state ads,
		    const struct pollfd *pollfds, int npollfds,
		    int maxfd, const fd_set *readfds,
		    const fd_set *writefds, const fd_set *exceptfds,
		    struct timeval now, int *r_r) {
  int i, revents;
  ADNS_SOCKET fd;

  for (i=0; i<npollfds; i++) {
    fd= pollfds[i].fd;
    if ((int)fd >= maxfd) maxfd= fd+1;
    revents= pollfds[i].revents;
    fd_event(ads,fd, revents,POLLIN, maxfd,readfds, adns_processreadable,now,r_r);
    fd_event(ads,fd, revents,POLLOUT, maxfd,writefds, adns_processwriteable,now,r_r);
    fd_event(ads,fd, revents,POLLPRI, maxfd,exceptfds, adns_processexceptional,now,r_r);
  }
}

/* Wrappers for select(2). */

void adns_beforeselect(adns_state ads, int *maxfd_io, fd_set *readfds_io,
		       fd_set *writefds_io, fd_set *exceptfds_io,
		       struct timeval **tv_mod, struct timeval *tv_tobuf,
		       const struct timeval *now) {
  struct timeval tv_nowbuf;
  struct pollfd pollfds[MAX_POLLFDS];
  int i, maxfd, npollfds;
  ADNS_SOCKET fd;
  
  adns__consistency(ads,0,cc_entex);

  if (tv_mod && (!*tv_mod || (*tv_mod)->tv_sec || (*tv_mod)->tv_usec)) {
    /* The caller is planning to sleep. */
    adns__must_gettimeofday(ads,&now,&tv_nowbuf);
    if (!now) { inter_immed(tv_mod,tv_tobuf); goto xit; }
    adns__timeouts(ads, 0, tv_mod,tv_tobuf, *now);
  }

  npollfds= adns__pollfds(ads,pollfds);
  maxfd= *maxfd_io;
  for (i=0; i<npollfds; i++) {
    fd= pollfds[i].fd;
    if ((int)fd >= maxfd) maxfd= fd+1;
    if (pollfds[i].events & POLLIN) FD_SET(fd,readfds_io);
    if (pollfds[i].events & POLLOUT) FD_SET(fd,writefds_io);
    if (pollfds[i].events & POLLPRI) FD_SET(fd,exceptfds_io);
  }
  *maxfd_io= maxfd;

xit:
  adns__consistency(ads,0,cc_entex);
}

void adns_afterselect(adns_state ads, int maxfd, const fd_set *readfds,
		      const fd_set *writefds, const fd_set *exceptfds,
		      const struct timeval *now) {
  struct timeval tv_buf;
  struct pollfd pollfds[MAX_POLLFDS];
  int npollfds, i;

  adns__consistency(ads,0,cc_entex);
  adns__must_gettimeofday(ads,&now,&tv_buf);
  if (!now) goto xit;
  adns_processtimeouts(ads,now);

  npollfds= adns__pollfds(ads,pollfds);
  for (i=0; i<npollfds; i++) pollfds[i].revents= POLLIN|POLLOUT|POLLPRI;
  adns__fdevents(ads,
		 pollfds,npollfds,
		 maxfd,readfds,writefds,exceptfds,
		 *now, 0);
xit:
  adns__consistency(ads,0,cc_entex);
}

/* General helpful functions. */

void adns_globalsystemfailure(adns_state ads) {
  adns__consistency(ads,0,cc_entex);

  while (ads->udpw.head) adns__query_fail(ads->udpw.head, adns_s_systemfail);
  while (ads->tcpw.head) adns__query_fail(ads->tcpw.head, adns_s_systemfail);
  
  switch (ads->tcpstate) {
  case server_connecting:
  case server_ok:
    adns__tcp_broken(ads,0,0);
    break;
  case server_disconnected:
  case server_broken:
    break;
  default:
    abort();
  }
  adns__consistency(ads,0,cc_entex);
}

int adns_processany(adns_state ads) {
  int r, i;
  struct timeval now;
  struct pollfd pollfds[MAX_POLLFDS];
  int npollfds;

  adns__consistency(ads,0,cc_entex);

  r= gettimeofday(&now,0);
  if (!r) adns_processtimeouts(ads,&now);

  /* We just use adns__fdevents to loop over the fd's trying them.
   * This seems more sensible than calling select, since we're most
   * likely just to want to do a adns_socket_read on one or two fds anyway.
   */
  npollfds= adns__pollfds(ads,pollfds);
  for (i=0; i<npollfds; i++) pollfds[i].revents= pollfds[i].events & ~POLLPRI;
  adns__fdevents(ads,
		 pollfds,npollfds,
		 0,0,0,0,
		 now,&r);

  adns__consistency(ads,0,cc_entex);
  return 0;
}

void adns__autosys(adns_state ads, struct timeval now) {
  if (ads->iflags & adns_if_noautosys) return;
  adns_processany(ads);
}

int adns__internal_check(adns_state ads,
			 adns_query *query_io,
			 adns_answer **answer,
			 void **context_r) {
  adns_query qu;

  qu= *query_io;
  if (!qu) {
    if (ads->output.head) {
      qu= ads->output.head;
    } else if (ads->udpw.head || ads->tcpw.head) {
      return EAGAIN;
    } else {
      return ESRCH;
    }
  } else {
    if (qu->id>=0) return EAGAIN;
  }
  LIST_UNLINK(ads->output,qu);
  *answer= qu->answer;
  if (context_r) *context_r= qu->ctx.ext;
  *query_io= qu;
  free(qu);
  return 0;
}

int adns_wait(adns_state ads,
	      adns_query *query_io,
	      adns_answer **answer_r,
	      void **context_r) {
  int r, maxfd, rsel;
  fd_set readfds, writefds, exceptfds;
  struct timeval tvbuf, *tvp;
  
  adns__consistency(ads,*query_io,cc_entex);
  for (;;) {
    r= adns__internal_check(ads,query_io,answer_r,context_r);
    if (r != EAGAIN) break;
    maxfd= 0; tvp= 0;
    FD_ZERO(&readfds); FD_ZERO(&writefds); FD_ZERO(&exceptfds);
    adns_beforeselect(ads,&maxfd,&readfds,&writefds,&exceptfds,&tvp,&tvbuf,0);
    assert(tvp);
	ADNS_CLEAR_ERRNO;
    rsel= select(maxfd,&readfds,&writefds,&exceptfds,tvp);
	ADNS_CAPTURE_ERRNO;
    if (rsel==-1) {
      if (errno == EINTR) {
	if (ads->iflags & adns_if_eintr) { r= EINTR; break; }
      } else {
	adns__diag(ads,-1,0,"select failed in wait: %s",strerror(errno));
	adns_globalsystemfailure(ads);
      }
    } else {
      assert(rsel >= 0);
      adns_afterselect(ads,maxfd,&readfds,&writefds,&exceptfds,0);
    }
  }
  adns__consistency(ads,0,cc_entex);
  return r;
}

int adns_check(adns_state ads,
	       adns_query *query_io,
	       adns_answer **answer_r,
	       void **context_r) {
  struct timeval now;
  int r;
  
  adns__consistency(ads,*query_io,cc_entex);
  r= gettimeofday(&now,0);
  if (!r) adns__autosys(ads,now);

  r= adns__internal_check(ads,query_io,answer_r,context_r);
  adns__consistency(ads,0,cc_entex);
  return r;
}
