/*
 * adh-main.c
 * - useful general-purpose resolver client program
 *   main program and useful subroutines
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
# include <io.h>
#endif

#include "adnshost.h"

int rcode;
const char *config_text;

static int used, avail;
static char *buf;

void quitnow(int rc) {
  if (ads) adns_finish(ads);
  free(buf);
  free(ov_id);
  exit(rc);
}

void sysfail(const char *what, int errnoval) {
  fprintf(stderr,"adnshost failed: %s: %s\n",what,strerror(errnoval));
  quitnow(10);
}

void usageerr(const char *fmt, ...) {
  va_list al;
  fputs("adnshost usage error: ",stderr);
  va_start(al,fmt);
  vfprintf(stderr,fmt,al);
  va_end(al);
  putc('\n',stderr);
  quitnow(11);
}

void outerr(void) {
  sysfail("write to stdout",errno);
}

void *xmalloc(size_t sz) {
  void *p;

  p= malloc(sz); if (!p) sysfail("malloc",sz);
  return p;
}

char *xstrsave(const char *str) {
  char *p;
  
  p= xmalloc(strlen(str)+1);
  strcpy(p,str);
  return p;
}

void of_config(const struct optioninfo *oi, const char *arg, const char *arg2) {
  config_text= arg;
}

void of_type(const struct optioninfo *oi, const char *arg, const char *arg2) {
  static const struct typename {
    adns_rrtype type;
    const char *desc;
  } typenames[]= {
    /* enhanced versions */
    { adns_r_ns,     "ns"     },
    { adns_r_soa,    "soa"    },
    { adns_r_ptr,    "ptr"    },
    { adns_r_mx,     "mx"     },
    { adns_r_rp,     "rp"     },
    { adns_r_addr,   "addr"   },
    
    /* types with only one version */
    { adns_r_cname,  "cname"  },
    { adns_r_hinfo,  "hinfo"  },
    { adns_r_txt,    "txt"    },
    
    /* raw versions */
    { adns_r_a,        "a"    },
    { adns_r_ns_raw,   "ns-"  },
    { adns_r_soa_raw,  "soa-" },
    { adns_r_ptr_raw,  "ptr-" },
    { adns_r_mx_raw,   "mx-"  },
    { adns_r_rp_raw,   "rp-"  },

    { adns_r_none, 0 }
  };

  const struct typename *tnp;

  for (tnp=typenames;
       tnp->type && strcmp(arg,tnp->desc);
       tnp++);
  if (!tnp->type) usageerr("unknown RR type %s",arg);
  ov_type= tnp->type;
}

static void process_optarg(const char *arg,
			   const char *const **argv_p,
			   const char *value) {
  const struct optioninfo *oip;
  const char *arg2;
  int invert;

  if (arg[0] == '-' || arg[0] == '+') {
    if (arg[0] == '-' && arg[1] == '-') {
      if (!strncmp(arg,"--no-",5)) {
	invert= 1;
	oip= opt_findl(arg+5);
      } else {
	invert= 0;
	oip= opt_findl(arg+2);
      }
      if (oip->type == ot_funcarg) {
	arg= argv_p ? *++(*argv_p) : value;
	if (!arg) usageerr("option --%s requires a value argument",oip->lopt);
	arg2= 0;
      } else if (oip->type == ot_funcarg2) {
	assert(argv_p);
	arg= *++(*argv_p);
	arg2= arg ? *++(*argv_p) : 0;
	if (!arg || !arg2)
	  usageerr("option --%s requires two more arguments", oip->lopt);
      } else {
	if (value) usageerr("option --%s does not take a value",oip->lopt);
	arg= 0;
	arg2= 0;
      }
      opt_do(oip,invert,arg,arg2);
    } else if (arg[0] == '-' && arg[1] == 0) {
      arg= argv_p ? *++(*argv_p) : value;
      if (!arg) usageerr("option `-' must be followed by a domain");
      query_do(arg);
    } else { /* arg[1] != '-', != '\0' */
      invert= (arg[0] == '+');
      ++arg;
      while (*arg) {
	oip= opt_finds(&arg);
	if (oip->type == ot_funcarg) {
	  if (!*arg) {
	    arg= argv_p ? *++(*argv_p) : value;
	    if (!arg) usageerr("option -%s requires a value argument",oip->sopt);
	  } else {
	    if (value) usageerr("two values for option -%s given !",oip->sopt);
	  }
	  opt_do(oip,invert,arg,0);
	  arg= "";
	} else {
	  if (value) usageerr("option -%s does not take a value",oip->sopt);
	  opt_do(oip,invert,0,0);
	}
      }
    }
  } else { /* arg[0] != '-' */
    query_do(arg);
  }
}
    
static void read_stdin(void) {
  int anydone, r;
  char *newline, *space;

  anydone= 0;
  while (!anydone || used) {
    while (!(newline= memchr(buf,'\n',used))) {
      if (used == avail) {
	avail += 20; avail <<= 1;
	buf= realloc(buf,avail);
	if (!buf) sysfail("realloc stdin buffer",errno);
      }
      do {
	r= read(0,buf+used,avail-used);
      } while (r < 0 && errno == EINTR);
      if (r == 0) {
	if (used) {
	  /* fake up final newline */
	  buf[used++]= '\n';
	  r= 1;
	} else {
	  ov_pipe= 0;
	  return;
	}
      }
      if (r < 0) sysfail("read stdin",errno);
      used += r;
    }
    *newline++= 0;
    space= strchr(buf,' ');
    if (space) *space++= 0;
    process_optarg(buf,0,space);
    used -= (newline-buf);
    memmove(buf,newline,used);
    anydone= 1;
  }
}

int main(int argc, const char *const *argv) {
  struct timeval *tv, tvbuf;
  adns_query qu;
  void *qun_v;
  adns_answer *answer;
  int r, maxfd;
  fd_set readfds, writefds, exceptfds;
  const char *arg;

  ensure_adns_init(); 
  
  while ((arg= *++argv)) process_optarg(arg,&argv,0);

  if (!ov_pipe && !ads) usageerr("no domains given, and -f/--pipe not used; try --help");

  for (;;) {
    for (;;) {
      qu= ov_asynch ? 0 : outstanding.head ? outstanding.head->qu : 0;
      r= adns_check(ads,&qu,&answer,&qun_v);
      if (r == EAGAIN) break;
      if (r == ESRCH) { if (!ov_pipe) goto x_quit; else break; }
      assert(!r);
      query_done(qun_v,answer);
    }
    maxfd= 0;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    if (ov_pipe) {
      maxfd= 1;
      FD_SET(0,&readfds);
    }
    tv= 0;
    adns_beforeselect(ads, &maxfd, &readfds,&writefds,&exceptfds, &tv,&tvbuf,0);
	ADNS_CLEAR_ERRNO;
    r= select(maxfd, &readfds,&writefds,&exceptfds, tv);
	ADNS_CAPTURE_ERRNO;
    if (r == -1) {
		ADNS_CAPTURE_ERRNO;
      if (errno == EINTR) continue;
      sysfail("select",errno);
    }
    adns_afterselect(ads, maxfd, &readfds,&writefds,&exceptfds, 0);
    if (ov_pipe && FD_ISSET(0,&readfds)) read_stdin();
  }
x_quit:
  if (fclose(stdout)) outerr();
  quitnow(rcode);
}
