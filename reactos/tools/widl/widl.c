/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 * based on WRC code by Bertho Stultiens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "wine/wpp.h"
#include "header.h"

/* future options to reserve characters for: */
/* a = alignment of structures */
/* A = ACF input filename */
/* J = do not search standard include path */
/* O = generate interpreted stubs */
/* u = UUID file only? */
/* U = UUID filename */
/* w = select win16/win32 output (?) */

static char usage[] =
"Usage: widl [options...] infile.idl\n"
"   -c          Generate client stub\n"
"   -C file     Name of client stub file (default is infile_c.c)\n"
"   -d n        Set debug level to 'n'\n"
"   -D id[=val] Define preprocessor identifier id=val\n"
"   -E          Preprocess only\n"
"   -h          Generate headers\n"
"   -H file     Name of header file (default is infile.h)\n"
"   -I path     Set include search dir to path (multiple -I allowed)\n"
"   -N          Do not preprocess input\n"
"   --oldnames  Use old naming conventions\n"
"   -p          Generate proxy\n"
"   -P file     Name of proxy file (default is infile_p.c)\n"
"   -s          Generate server stub\n"
"   -S file     Name of server stub file (default is infile_s.c)\n"
"   -t          Generate typelib\n"
"   -T file     Name of typelib file (default is infile.tlb)\n"
"   -V          Print version and exit\n"
"   -W          Enable pedantic warnings\n"
"Debug level 'n' is a bitmask with following meaning:\n"
"    * 0x01 Tell which resource is parsed (verbose mode)\n"
"    * 0x02 Dump internal structures\n"
"    * 0x04 Create a parser trace (yydebug=1)\n"
"    * 0x08 Preprocessor messages\n"
"    * 0x10 Preprocessor lex messages\n"
"    * 0x20 Preprocessor yacc trace\n"
;

static const char version_string[] = "Wine IDL Compiler version " PACKAGE_VERSION "\n"
			"Copyright 2002 Ove Kaaven\n";

int win32 = 1;
int debuglevel = DEBUGLEVEL_NONE;
int yy_flex_debug;

int pedantic = 0;
static int do_everything = 1;
int preprocess_only = 0;
int do_header = 0;
int do_typelib = 0;
int do_proxies = 0;
int do_client = 0;
int do_server = 0;
int no_preprocess = 0;
int old_names = 0;

char *input_name;
char *header_name;
char *header_token;
char *typelib_name;
char *proxy_name;
char *proxy_token;
char *client_name;
char *client_token;
char *server_name;
char *server_token;
char *temp_name;

int line_number = 1;

FILE *header;
FILE *proxy;

time_t now;

static const char *short_options =
    "cC:d:D:EhH:I:NpP:sS:tT:VW";
static struct option long_options[] = {
    { "oldnames", 0, 0, 1 },
    { 0, 0, 0, 0 }
};

static void rm_tempfile(void);
static void segvhandler(int sig);

static char *make_token(const char *name)
{
  char *token;
  char *slash;
  int i;

  slash = strrchr(name, '/');
  if (slash) name = slash + 1;

  token = xstrdup(name);
  for (i=0; token[i]; i++) {
    if (!isalnum(token[i])) token[i] = '_';
    else token[i] = toupper(token[i]);
  }
  return token;
}

/* duplicate a basename into a valid C token */
static char *dup_basename_token(const char *name, const char *ext)
{
    char *p, *ret = dup_basename( name, ext );
    /* map invalid characters to '_' */
    for (p = ret; *p; p++) if (!isalnum(*p)) *p = '_';
    return ret;
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

int main(int argc,char *argv[])
{
  extern char* optarg;
  extern int   optind;
  int optc;
  int ret = 0;
  int opti = 0;

  signal(SIGSEGV, segvhandler);
  signal( SIGTERM, exit_on_signal );
  signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
  signal( SIGHUP, exit_on_signal );
#endif

  now = time(NULL);

  while((optc = getopt_long(argc, argv, short_options, long_options, &opti)) != EOF) {
    switch(optc) {
    case 1:
      old_names = 1;
      break;
    case 'c':
      do_everything = 0;
      do_client = 1;
      break;
    case 'C':
      client_name = strdup(optarg);
      break;
    case 'd':
      debuglevel = strtol(optarg, NULL, 0);
      break;
    case 'D':
      wpp_add_cmdline_define(optarg);
      break;
    case 'E':
      do_everything = 0;
      preprocess_only = 1;
      break;
    case 'h':
      do_everything = 0;
      do_header = 1;
      break;
    case 'H':
      header_name = strdup(optarg);
      break;
    case 'I':
      wpp_add_include_path(optarg);
      break;
    case 'N':
      no_preprocess = 1;
      break;
    case 'p':
      do_everything = 0;
      do_proxies = 1;
      break;
    case 'P':
      proxy_name = strdup(optarg);
      break;
    case 's':
      do_everything = 0;
      do_server = 1;
      break;
    case 'S':
      server_name = strdup(optarg);
      break;
    case 't':
      do_everything = 0;
      do_typelib = 1;
      break;
    case 'T':
      typelib_name = strdup(optarg);
      break;
    case 'V':
      printf(version_string);
      return 0;
    case 'W':
      pedantic = 1;
      break;
    default:
      fprintf(stderr, usage);
      return 1;
    }
  }

  if(do_everything) {
      do_header = do_typelib = do_proxies = do_client = do_server = 1;
  }
  if(optind < argc) {
    input_name = xstrdup(argv[optind]);
  }
  else {
    fprintf(stderr, usage);
    return 1;
  }

  if(debuglevel)
  {
    setbuf(stdout,0);
    setbuf(stderr,0);
  }

  yydebug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
  yy_flex_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;

  wpp_set_debug( (debuglevel & DEBUGLEVEL_PPLEX) != 0,
                 (debuglevel & DEBUGLEVEL_PPTRACE) != 0,
                 (debuglevel & DEBUGLEVEL_PPMSG) != 0 );

  if (!header_name) {
    header_name = dup_basename(input_name, ".idl");
    strcat(header_name, ".h");
  }

  if (!typelib_name && do_typelib) {
    typelib_name = dup_basename(input_name, ".idl");
    strcat(typelib_name, ".tlb");
  }

  if (!proxy_name && do_proxies) {
    proxy_name = dup_basename(input_name, ".idl");
    strcat(proxy_name, "_p.c");
  }

  if (!client_name && do_client) {
    client_name = dup_basename(input_name, ".idl");
    strcat(client_name, "_c.c");
  }

  if (!server_name && do_server) {
    server_name = dup_basename(input_name, ".idl");
    strcat(server_name, "_s.c");
  }

  if (do_proxies) proxy_token = dup_basename_token(proxy_name,"_p.c");
  if (do_client) client_token = dup_basename_token(client_name,"_c.c");
  if (do_server) server_token = dup_basename_token(server_name,"_s.c");

  wpp_add_cmdline_define("__WIDL__");

  atexit(rm_tempfile);
  if (!no_preprocess)
  {
    chat("Starting preprocess\n");

    if (!preprocess_only)
    {
        ret = wpp_parse_temp( input_name, header_name, &temp_name );
    }
    else
    {
        ret = wpp_parse( input_name, stdout );
    }

    if(ret) exit(1);
    if(preprocess_only) exit(0);
    if(!(yyin = fopen(temp_name, "r"))) {
      fprintf(stderr, "Could not open %s for input\n", temp_name);
      return 1;
    }
  }
  else {
    if(!(yyin = fopen(input_name, "r"))) {
      fprintf(stderr, "Could not open %s for input\n", input_name);
      return 1;
    }
  }

  if(do_header) {
    header_token = make_token(header_name);

    if(!(header = fopen(header_name, "w"))) {
      fprintf(stderr, "Could not open %s for output\n", header_name);
      return 1;
    }
    fprintf(header, "/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
    fprintf(header, "#include <rpc.h>\n" );
    fprintf(header, "#include <rpcndr.h>\n\n" );
    fprintf(header, "#ifndef __WIDL_%s\n", header_token);
    fprintf(header, "#define __WIDL_%s\n", header_token);
    fprintf(header, "#ifdef __cplusplus\n");
    fprintf(header, "extern \"C\" {\n");
    fprintf(header, "#endif\n");
  }

  ret = yyparse();

  if(do_header) {
    fprintf(header, "/* Begin additional prototypes for all interfaces */\n");
    fprintf(header, "\n");
    write_user_types();
    fprintf(header, "\n");
    fprintf(header, "/* End additional prototypes */\n");
    fprintf(header, "\n");
    fprintf(header, "#ifdef __cplusplus\n");
    fprintf(header, "}\n");
    fprintf(header, "#endif\n");
    fprintf(header, "#endif /* __WIDL_%s */\n", header_token);
    fclose(header);
  }

  fclose(yyin);

  if(ret) {
    exit(1);
  }
  header_name = NULL;
  client_name = NULL;
  server_name = NULL;
  return 0;
}

static void rm_tempfile(void)
{
  abort_import();
  if(temp_name)
    unlink(temp_name);
  if (header_name)
    unlink(header_name);
  if (client_name)
    unlink(client_name);
  if (server_name)
    unlink(server_name);
}

static void segvhandler(int sig)
{
  fprintf(stderr, "\n%s:%d: Oops, segment violation\n", input_name, line_number);
  fflush(stdout);
  fflush(stderr);
  abort();
}
