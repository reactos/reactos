/*
 * adh-opts.c
 * - useful general-purpose resolver client program
 *   option handling tables etc.
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

#include "adnshost.h"

int ov_env=1, ov_pipe=0, ov_asynch=0;
int ov_verbose= 0;
adns_rrtype ov_type= adns_r_none;
int ov_search=0, ov_qc_query=0, ov_qc_anshost=0, ov_qc_cname=1;
int ov_tcp=0, ov_cname=0, ov_format=fmt_default;
char *ov_id= 0;
struct perqueryflags_remember ov_pqfr = { 1,1,1, tm_none };

static const struct optioninfo global_options[]= {
  { ot_desconly, "global binary options:" },
  { ot_flag,             "Do not look at environment variables at all",
    "e", "env",            &ov_env, 0 },
  { ot_flag,             "Read queries on stdin instead of using args",
    "f", "pipe",           &ov_pipe, 1 },
  { ot_flag,             "Allow answers to be reordered",
    "a", "asynch",         &ov_asynch, 1 },
  
  { ot_desconly, "answer/error output format and destination (see below):" },
  { ot_value,            "Answers to stdout, errors as messages to stderr (default)",
    "Fs", "fmt-simple",    &ov_format, fmt_simple },
  { ot_value,            "Answers and errors both to stdout in parseable format",
    "Fi", "fmt-inline",    &ov_format, fmt_inline },
  { ot_value,            "Fully-parseable output format (default for --asynch)",
    "Fa", "fmt-asynch",    &ov_format, fmt_asynch },
  		         
  { ot_desconly, "global verbosity level:" },
  { ot_value,            "Do not print anything to stderr",
    "Vq", "quiet",         &ov_verbose, adns_if_noerrprint },
  { ot_value,            "Report unexpected kinds of problem only  (default)",
    "Vn", "no-quiet",      &ov_verbose, 0 },
  { ot_value,            "Debugging mode",
    "Vd", "debug",         &ov_verbose, adns_if_debug },
  		         
  { ot_desconly, "other global options:" },
  { ot_funcarg,          "Configuration to use instead of /etc/resolv.conf",
    0, "config",           0,0, of_config, "<config-text>" },
  { ot_func,             "Print version number",
    0, "version",          0,0, of_version },
  { ot_func,             "Print usage information",
    0, "help",             0,0, of_help },

  { ot_end }
};

static const struct optioninfo perquery_options[]= {
  { ot_desconly, "per-query options:" },
  { ot_funcarg,          "Query type (see below)",
    "t", "type",           0,0, &of_type, "type" },
  { ot_funcarg,          "Do reverse query (address -> name lookup)",
    "i", "ptr",            0,0, &of_ptr, "addr" },
  { ot_funcarg2,         "Lookup in in-addr-like `zone' (eg MAPS RBL)",
    0, "reverse",          0,0, &of_reverse, "addr","zone" },

  { ot_desconly, "per-query binary options:" },
  { ot_flag,             "Use the search list",
    "s", "search",         &ov_search, 1 },
  { ot_flag,             "Let query domains contain quote-requiring chars",
    "Qq", "qc-query",      &ov_qc_query, 1 },
  { ot_flag,             "Let hostnames in answers contain ...",
    "Qa", "qc-anshost",    &ov_qc_anshost, 1 },
  { ot_flag,             "Prevent CNAME target domains from containing ...",
    "Qc", "qc-cname",      &ov_qc_cname, 0 },
  { ot_flag,             "Force use of a virtual circuit",
    "u", "tcp",            &ov_tcp, 1 },
  { ot_flag,             "Do not display owner name in output",
    "Do", "show-owner",   &ov_pqfr.show_owner, 0 },
  { ot_flag,             "Do not display RR type in output",
    "Dt", "show-type",    &ov_pqfr.show_type, 0 },
  { ot_flag,             "Do not display CNAME target in output",
    "Dc", "show-cname",    &ov_pqfr.show_cname, 0 },
  
  { ot_desconly, "per-query TTL mode (NB TTL is minimum across all info in reply):" },
  { ot_value,            "Show the TTL as a TTL",
    "Tt", "ttl-ttl",       &ov_pqfr.ttl, tm_rel },
  { ot_value,            "Show the TTL as a time_t when the data might expire",
    "Ta", "ttl-abs",       &ov_pqfr.ttl, tm_abs },
  { ot_value,            "Do not show the TTL (default)",
    "Tn", "no-ttl",        &ov_pqfr.ttl, tm_none },
  
  { ot_desconly, "per-query CNAME handling mode:" },
  { ot_value,            "Call it an error if a CNAME is found",
    "Cf", "cname-reject",  &ov_cname, adns_qf_cname_forbid },
  { ot_value,            "Allow references to CNAMEs in other RRs",
    "Cl", "cname-loose",   &ov_cname, adns_qf_cname_loose },
  { ot_value,            "CNAME ok for query domain, but not in RRs (default)",
    "Cs", "cname-ok",      &ov_cname, 0 },
  
  { ot_desconly, "asynchronous/pipe mode options:" },
  { ot_funcarg,          "Set <id>, default is decimal sequence starting 0",
    0, "asynch-id",        0,0, &of_asynch_id, "id" },
  { ot_funcarg,          "Cancel the query with id <id> (no error if not found)",
    0, "cancel-id",        0,0, &of_cancel_id, "id" },

  { ot_end }
};

static void printusage(void) {
  static const struct optioninfo *const all_optiontables[]= {
    global_options, perquery_options, 0
  };

  const struct optioninfo *const *oiap, *oip=0;
  int maxsopt, maxlopt, l;

  maxsopt= maxlopt= 0;
  
  for (oiap=all_optiontables; *oiap; oiap++) {
    for (oip=*oiap; oip->type != ot_end; oip++) {
      if (oip->type == ot_funcarg) continue;
      if (oip->sopt) { l= strlen(oip->sopt); if (l>maxsopt) maxsopt= l; }
      if (oip->lopt) {
	l= strlen(oip->lopt);
	if (oip->type == ot_flag && !oip->value) l+= 3;
	if (l>maxlopt) maxlopt= l;
      }
    }
  }
	
  fputs("usage: adnshost [global-opts] [query-opts] query-domain\n"
	"                             [[query-opts] query-domain ...]\n"
	"       adnshost [global-opts] [query-opts] -f|--pipe\n",
	stdout);

  for (oiap=all_optiontables; *oiap; oiap++) {
    putchar('\n');
    for (oip=*oiap; oip->type != ot_end; oip++) {
      switch (oip->type) {
      case ot_flag:
	if (!oip->value) {
	  if (oip->sopt) {
	    printf(" +%-*s --no-%-*s %s\n",
		   maxsopt, oip->sopt,
		   maxlopt-2, oip->lopt,
		   oip->desc);
	  } else {
	    printf(" --no-%-*s %s\n",
		   maxlopt+maxsopt+1, oip->lopt,
		   oip->desc);
	  }
	  break;
	}
      case ot_value: case ot_func: /* fall through */
	if (oip->sopt) {
	  printf(" -%-*s --%-*s %s\n",
		 maxsopt, oip->sopt,
		 maxlopt+1, oip->lopt,
		 oip->desc);
	} else {
	  printf(" --%-*s %s\n",
		 maxlopt+maxsopt+3, oip->lopt,
		 oip->desc);
	}
	break;
      case ot_funcarg:
	if (oip->sopt) {
	  l= (maxlopt + maxsopt - 9 -
	      (strlen(oip->sopt) + strlen(oip->lopt) + 2*strlen(oip->argdesc)));
	  printf(" -%s<%s> / --%s <%s>%*s%s\n",
		 oip->sopt, oip->argdesc, oip->lopt, oip->argdesc,
		 l>2 ? l : 2, "",
		 oip->desc);
	} else {
	  l= (maxlopt + maxsopt + 1 -
	      (strlen(oip->lopt) + strlen(oip->argdesc)));
	  printf(" --%s <%s>%*s%s\n",
		 oip->lopt, oip->argdesc,
		 l>2 ? l : 2, "",
		 oip->desc);
	}
	break;
      case ot_funcarg2:
	assert(!oip->sopt);
	l= (maxlopt + maxsopt - 2 -
	    (strlen(oip->lopt) + strlen(oip->argdesc) + strlen(oip->argdesc2)));
	  printf(" --%s <%s> <%s>%*s%s\n",
		 oip->lopt, oip->argdesc, oip->argdesc2,
		 l>2 ? l : 2, "",
		 oip->desc);
	break;
      case ot_desconly:
	printf("%s\n", oip->desc);
	break;
      default:
	abort();
      }
    }
  }

  printf("\nEscaping domains which might start with `-':\n"
	 " - %-*s Next argument is a domain, but more options may follow\n",
	 maxlopt+maxsopt+3, "<domain>");
  
  fputs("\n"
	"Query domains should always be quoted according to master file format.\n"
	"\n"
	"For binary options, --FOO and --no-FOO are opposites, as are\n"
	"-X and +X.  In each case the default is the one not listed.\n"
	"Per query options stay set a particular way until they are reset,\n"
	"whether they appear on the command line or on stdin.\n"
	"All global options must preceed the first query domain.\n"
	"\n"
	"With -f, the input should be lines with either an option, possibly\n"
	"with a value argument (separated from the option by a space if it's a long\n"
	"option), or a domain (possibly preceded by a hyphen and a space to\n"
	"distinguish it from an option).\n"
	"\n"
	"Output format is master file format without class or TTL by default:\n"
	"   [<owner>] [<ttl>] [<type>] <data>\n"
	"or if the <owner> domain refers to a CNAME and --show-cname is on\n"
	"   [<owner>] [<ttl>] CNAME <cname>\n"
	"   [<cname>] [<ttl>] <type> <data>\n"
	"When a query fails you get an error message to stderr (with --fmt-simple).\n"
	"Specify --fmt-inline for lines like this (broken here for readability):\n"
	"   ; failed <statustype> <statusnum> <statusabbrev> \\\n"
	"       [<owner>] [<ttl>] [<cname>] \"<status string>\"\n"
	"If you use --fmt-asynch, which is the default for --asynch,\n"
	"each answer (success or failure) is preceded by a line\n"
	"   <id> <nrrs> <statustype> <statusnum> <statusabbrev> \\\n"
	"       [<owner>] [<ttl>] [<cname>] \"<status string>\"\n"
	"where <nrrs> is the number of RRs that follow and <cname> will be `$' or\n"
	"the CNAME target; the CNAME indirection and error formats above are not used.\n"
	"\n"
	"Exit status:\n"
	" 0    all went well\n"
	" 1-6  at least one query failed with statustype:\n"
	"   1    localfail   )\n"
	"   2    remotefail  ) temporary errors\n"
	"   3    tempfail  __)_________________\n"
	"   4    misconfig   )\n"
	"   5    misquery    ) permanent errors\n"
	"   6    permfail    )\n"
	" 10   system trouble\n"
	" 11   usage problems\n"
	"\n"
	"Query types (see adns.h; default is addr):\n"
	"  ns  soa  ptr  mx  rp  addr       - enhanced versions\n"
	"  cname  hinfo  txt                - types with only one version\n"
	"  a  ns-  soa-  ptr-  mx-  rp-     - _raw versions\n"
	"Default is addr, or ptr for -i/--ptr queries\n",
	stdout);
  if (ferror(stdout)) sysfail("write usage message",errno);
}

void of_version(const struct optioninfo *oi, const char *arg, const char *arg2) {
  VERSION_PRINT_QUIT("adnshost");
}

void of_help(const struct optioninfo *oi, const char *arg, const char *arg2) {
  printusage();
  if (fclose(stdout)) sysfail("finish writing output",errno);
  quitnow(0);
}

typedef int comparer_type(const char **optp, const struct optioninfo *entry);

static int oc_long(const char **optp, const struct optioninfo *entry) {
  return entry->lopt && !strcmp(*optp,entry->lopt);
}

static int oc_short(const char **optp, const struct optioninfo *entry) {
  const char *sopt;
  int l;

  sopt= entry->sopt;
  if (!sopt) return 0;
  l= strlen(sopt);
  if (memcmp(*optp,sopt,l)) return 0;
  (*optp) += l;
  return 1;
}

static const struct optioninfo *find1(const char **optp,
				      const struct optioninfo *table,
				      comparer_type *comparer) {
  for (;;) {
    if (table->type == ot_end) return 0;
    if (comparer(optp,table)) return table;
    table++;
  }
}

static const struct optioninfo *find(const char **optp,
				     const char *prefix,
				     comparer_type *comparer) {
  const struct optioninfo *oip;
  const char *opt;

  opt= *optp;
  oip= find1(optp,perquery_options,comparer);
  if (oip) return oip;
  oip= find1(optp,global_options,comparer);
  if (!oip) usageerr("unknown option %s%s",prefix,opt);
  if (ads) usageerr("global option %s%s specified after query domain(s)",prefix,opt);
  return oip;
}

const struct optioninfo *opt_findl(const char *opt) { return find(&opt,"--",oc_long); }
const struct optioninfo *opt_finds(const char **optp) { return find(optp,"-",oc_short); }

static void noninvert(const struct optioninfo *oip) NONRETURNING;
static void noninvert(const struct optioninfo *oip) {
  usageerr("option %s%s%s%s%s may not be inverted",
	   oip->sopt ? "-" : "", oip->sopt ? oip->sopt : "",
	   oip->lopt && oip->sopt ? " / " : "",
	   oip->lopt ? "--" : "", oip->lopt ? oip->lopt : "");
}

void opt_do(const struct optioninfo *oip, int invert,
	    const char *arg, const char *arg2) {
  switch (oip->type) {
  case ot_flag:
    assert(!arg);
    *oip->storep= !invert;
    return;
  case ot_value:
    assert(!arg);
    if (invert) noninvert(oip);
    *oip->storep= oip->value;
    return;
  case ot_func: case ot_funcarg: case ot_funcarg2:
    if (invert) noninvert(oip);
    oip->func(oip,arg,arg2);
    return;
  default:
    abort();
  }
}
