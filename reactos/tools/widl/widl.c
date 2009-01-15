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

#include <errno.h>
#include <limits.h>
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
/* w = select win16/win32 output (?) */

static const char usage[] =
"Usage: widl [options...] infile.idl\n"
"   or: widl [options...] --dlldata-only name1 [name2...]\n"
"   -c          Generate client stub\n"
"   -C file     Name of client stub file (default is infile_c.c)\n"
"   -d n        Set debug level to 'n'\n"
"   -D id[=val] Define preprocessor identifier id=val\n"
"   --dlldata=file  Name of the dlldata file (default is dlldata.c)\n"
"   -E          Preprocess only\n"
"   -h          Generate headers\n"
"   -H file     Name of header file (default is infile.h)\n"
"   -I path     Set include search dir to path (multiple -I allowed)\n"
"   --local-stubs=file  Write empty stubs for call_as/local methods to file\n"
"   -N          Do not preprocess input\n"
"   --oldnames  Use old naming conventions\n"
"   -p          Generate proxy\n"
"   -P file     Name of proxy file (default is infile_p.c)\n"
"   --prefix-all=p  Prefix names of client stubs / server functions with 'p'\n"
"   --prefix-client=p  Prefix names of client stubs with 'p'\n"
"   --prefix-server=p  Prefix names of server functions with 'p'\n"
"   -s          Generate server stub\n"
"   -S file     Name of server stub file (default is infile_s.c)\n"
"   -t          Generate typelib\n"
"   -T file     Name of typelib file (default is infile.tlb)\n"
"   -u          Generate interface identifiers file\n"
"   -U file     Name of interface identifiers file (default is infile_i.c)\n"
"   -V          Print version and exit\n"
"   -W          Enable pedantic warnings\n"
"   --win32     Only generate 32-bit code\n"
"   --win64     Only generate 64-bit code\n"
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
int parser_debug, yy_flex_debug;

int pedantic = 0;
int do_everything = 1;
int preprocess_only = 0;
int do_header = 0;
int do_typelib = 0;
int do_proxies = 0;
int do_client = 0;
int do_server = 0;
int do_idfile = 0;
int do_dlldata = 0;
int no_preprocess = 0;
int old_names = 0;
int do_win32 = 1;
int do_win64 = 1;

char *input_name;
char *header_name;
char *local_stubs_name;
char *header_token;
char *typelib_name;
char *dlldata_name;
char *proxy_name;
char *proxy_token;
char *client_name;
char *client_token;
char *server_name;
char *server_token;
char *idfile_name;
char *idfile_token;
char *temp_name;
const char *prefix_client = "";
const char *prefix_server = "";

int line_number = 1;

FILE *header;
FILE *idfile;

size_t pointer_size = 0;

time_t now;

enum {
    OLDNAMES_OPTION = CHAR_MAX + 1,
    DLLDATA_OPTION,
    DLLDATA_ONLY_OPTION,
    LOCAL_STUBS_OPTION,
    PREFIX_ALL_OPTION,
    PREFIX_CLIENT_OPTION,
    PREFIX_SERVER_OPTION,
    WIN32_OPTION,
    WIN64_OPTION
};

static const char short_options[] =
    "cC:d:D:EhH:I:NpP:sS:tT:uU:VW";
static const struct option long_options[] = {
    { "dlldata", 1, 0, DLLDATA_OPTION },
    { "dlldata-only", 0, 0, DLLDATA_ONLY_OPTION },
    { "local-stubs", 1, 0, LOCAL_STUBS_OPTION },
    { "oldnames", 0, 0, OLDNAMES_OPTION },
    { "prefix-all", 1, 0, PREFIX_ALL_OPTION },
    { "prefix-client", 1, 0, PREFIX_CLIENT_OPTION },
    { "prefix-server", 1, 0, PREFIX_SERVER_OPTION },
    { "win32", 0, 0, WIN32_OPTION },
    { "win64", 0, 0, WIN64_OPTION },
    { 0, 0, 0, 0 }
};

static void rm_tempfile(void);

static char *make_token(const char *name)
{
  char *token;
  char *slash;
  int i;

  slash = strrchr(name, '/');
  if(!slash)
    slash = strrchr(name, '\\');

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

static void set_everything(int x)
{
  do_header = x;
  do_typelib = x;
  do_proxies = x;
  do_client = x;
  do_server = x;
  do_idfile = x;
  do_dlldata = x;
}

void start_cplusplus_guard(FILE *fp)
{
  fprintf(fp, "#ifdef __cplusplus\n");
  fprintf(fp, "extern \"C\" {\n");
  fprintf(fp, "#endif\n\n");
}

void end_cplusplus_guard(FILE *fp)
{
  fprintf(fp, "#ifdef __cplusplus\n");
  fprintf(fp, "}\n");
  fprintf(fp, "#endif\n\n");
}

typedef struct
{
  char *filename;
  struct list link;
} filename_node_t;

static void add_filename_node(struct list *list, const char *name)
{
  filename_node_t *node = xmalloc(sizeof *node);
  node->filename = dup_basename( name, ".idl" );
  list_add_tail(list, &node->link);
}

static void free_filename_nodes(struct list *list)
{
  filename_node_t *node, *next;
  LIST_FOR_EACH_ENTRY_SAFE(node, next, list, filename_node_t, link) {
    list_remove(&node->link);
    free(node->filename);
    free(node);
  }
}

static void write_dlldata_list(struct list *filenames)
{
  FILE *dlldata;
  filename_node_t *node;

  dlldata = fopen(dlldata_name, "w");
  if (!dlldata)
    error("couldn't open %s: %s\n", dlldata_name, strerror(errno));

  fprintf(dlldata, "/*** Autogenerated by WIDL %s ", PACKAGE_VERSION);
  fprintf(dlldata, "- Do not edit ***/\n\n");
  fprintf(dlldata, "#include <objbase.h>\n");
  fprintf(dlldata, "#include <rpcproxy.h>\n\n");
  start_cplusplus_guard(dlldata);

  LIST_FOR_EACH_ENTRY(node, filenames, filename_node_t, link)
    fprintf(dlldata, "EXTERN_PROXY_FILE(%s)\n", node->filename);

  fprintf(dlldata, "\nPROXYFILE_LIST_START\n");
  fprintf(dlldata, "/* Start of list */\n");
  LIST_FOR_EACH_ENTRY(node, filenames, filename_node_t, link)
    fprintf(dlldata, "  REFERENCE_PROXY_FILE(%s),\n", node->filename);
  fprintf(dlldata, "/* End of list */\n");
  fprintf(dlldata, "PROXYFILE_LIST_END\n\n");

  fprintf(dlldata, "DLLDATA_ROUTINES(aProxyFileList, GET_DLL_CLSID)\n\n");
  end_cplusplus_guard(dlldata);
  fclose(dlldata);
}

static char *eat_space(char *s)
{
  while (isspace((unsigned char) *s))
    ++s;
  return s;
}

void write_dlldata(const statement_list_t *stmts)
{
  struct list filenames = LIST_INIT(filenames);
  filename_node_t *node;
  FILE *dlldata;

  if (!do_dlldata || !need_proxy_file(stmts))
    return;

  dlldata = fopen(dlldata_name, "r");
  if (dlldata) {
    static char marker[] = "REFERENCE_PROXY_FILE";
    char *line = NULL;
    size_t len = 0;

    while (widl_getline(&line, &len, dlldata)) {
      char *start, *end;
      start = eat_space(line);
      if (strncmp(start, marker, sizeof marker - 1) == 0) {
        start = eat_space(start + sizeof marker - 1);
        if (*start != '(')
          continue;
        end = start = eat_space(start + 1);
        while (*end && *end != ')')
          ++end;
        if (*end != ')')
          continue;
        while (isspace((unsigned char) end[-1]))
          --end;
        *end = '\0';
        if (start < end)
          add_filename_node(&filenames, start);
      }
    }

    if (ferror(dlldata))
      error("couldn't read from %s: %s\n", dlldata_name, strerror(errno));

    free(line);
    fclose(dlldata);
  }

  LIST_FOR_EACH_ENTRY(node, &filenames, filename_node_t, link)
    if (strcmp(proxy_token, node->filename) == 0) {
      /* We're already in the list, no need to regenerate this file.  */
      free_filename_nodes(&filenames);
      return;
    }

  add_filename_node(&filenames, proxy_token);
  write_dlldata_list(&filenames);
  free_filename_nodes(&filenames);
}

static void write_id_data_stmts(const statement_list_t *stmts)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    if (stmt->type == STMT_TYPE)
    {
      const type_t *type = stmt->u.type;
      if (type->type == RPC_FC_IP)
      {
        const UUID *uuid;
        if (!is_object(type->attrs) && !is_attr(type->attrs, ATTR_DISPINTERFACE))
          continue;
        uuid = get_attrp(type->attrs, ATTR_UUID);
        write_guid(idfile, is_attr(type->attrs, ATTR_DISPINTERFACE) ? "DIID" : "IID",
                   type->name, uuid);
      }
      else if (type->type == RPC_FC_COCLASS)
      {
        const UUID *uuid = get_attrp(type->attrs, ATTR_UUID);
        write_guid(idfile, "CLSID", type->name, uuid);
      }
    }
    else if (stmt->type == STMT_LIBRARY)
    {
      const UUID *uuid = get_attrp(stmt->u.lib->attrs, ATTR_UUID);
      write_guid(idfile, "LIBID", stmt->u.lib->name, uuid);
      write_id_data_stmts(stmt->u.lib->stmts);
    }
  }
}

void write_id_data(const statement_list_t *stmts)
{
  if (!do_idfile) return;

  idfile_token = make_token(idfile_name);

  idfile = fopen(idfile_name, "w");
  if (! idfile) {
    error("Could not open %s for output\n", idfile_name);
    return;
  }

  fprintf(idfile, "/*** Autogenerated by WIDL %s ", PACKAGE_VERSION);
  fprintf(idfile, "from %s - Do not edit ***/\n\n", input_name);
  fprintf(idfile, "#include <rpc.h>\n");
  fprintf(idfile, "#include <rpcndr.h>\n\n");
  fprintf(idfile, "#include <initguid.h>\n\n");
  start_cplusplus_guard(idfile);

  write_id_data_stmts(stmts);

  fprintf(idfile, "\n");
  end_cplusplus_guard(idfile);

  fclose(idfile);
}

int main(int argc,char *argv[])
{
  extern char* optarg;
  extern int   optind;
  int optc;
  int ret = 0;
  int opti = 0;

  signal( SIGTERM, exit_on_signal );
  signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
  signal( SIGHUP, exit_on_signal );
#endif

  now = time(NULL);

  while((optc = getopt_long(argc, argv, short_options, long_options, &opti)) != EOF) {
    switch(optc) {
    case DLLDATA_OPTION:
      dlldata_name = xstrdup(optarg);
      break;
    case DLLDATA_ONLY_OPTION:
      do_everything = 0;
      do_dlldata = 1;
      break;
    case LOCAL_STUBS_OPTION:
      do_everything = 0;
      local_stubs_name = xstrdup(optarg);
      break;
    case OLDNAMES_OPTION:
      old_names = 1;
      break;
    case PREFIX_ALL_OPTION:
      prefix_client = xstrdup(optarg);
      prefix_server = xstrdup(optarg);
      break;
    case PREFIX_CLIENT_OPTION:
      prefix_client = xstrdup(optarg);
      break;
    case PREFIX_SERVER_OPTION:
      prefix_server = xstrdup(optarg);
      break;
    case WIN32_OPTION:
      do_win32 = 1;
      do_win64 = 0;
      break;
    case WIN64_OPTION:
      do_win32 = 0;
      do_win64 = 1;
      break;
    case 'c':
      do_everything = 0;
      do_client = 1;
      break;
    case 'C':
      client_name = xstrdup(optarg);
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
      header_name = xstrdup(optarg);
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
      proxy_name = xstrdup(optarg);
      break;
    case 's':
      do_everything = 0;
      do_server = 1;
      break;
    case 'S':
      server_name = xstrdup(optarg);
      break;
    case 't':
      do_everything = 0;
      do_typelib = 1;
      break;
    case 'T':
      typelib_name = xstrdup(optarg);
      break;
    case 'u':
      do_everything = 0;
      do_idfile = 1;
      break;
    case 'U':
      idfile_name = xstrdup(optarg);
      break;
    case 'V':
      printf("%s", version_string);
      return 0;
    case 'W':
      pedantic = 1;
      break;
    default:
      fprintf(stderr, "%s", usage);
      return 1;
    }
  }

  if(do_everything) {
    set_everything(TRUE);
  }

  if (!dlldata_name && do_dlldata)
    dlldata_name = xstrdup("dlldata.c");

  if(optind < argc) {
    if (do_dlldata && !do_everything) {
      struct list filenames = LIST_INIT(filenames);
      for ( ; optind < argc; ++optind)
        add_filename_node(&filenames, argv[optind]);

      write_dlldata_list(&filenames);
      free_filename_nodes(&filenames);
      return 0;
    }
    else if (optind != argc - 1) {
      fprintf(stderr, "%s", usage);
      return 1;
    }
    else
      input_name = xstrdup(argv[optind]);
  }
  else {
    fprintf(stderr, "%s", usage);
    return 1;
  }

  if(debuglevel)
  {
    setbuf(stdout,0);
    setbuf(stderr,0);
  }

  parser_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
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

  if (!idfile_name && do_idfile) {
    idfile_name = dup_basename(input_name, ".idl");
    strcat(idfile_name, "_i.c");
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
    if(!(parser_in = fopen(temp_name, "r"))) {
      fprintf(stderr, "Could not open %s for input\n", temp_name);
      return 1;
    }
  }
  else {
    if(!(parser_in = fopen(input_name, "r"))) {
      fprintf(stderr, "Could not open %s for input\n", input_name);
      return 1;
    }
  }

  header_token = make_token(header_name);

  init_types();
  ret = parser_parse();

  fclose(parser_in);

  if(ret) {
    exit(1);
  }

  /* Everything has been done successfully, don't delete any files.  */
  set_everything(FALSE);
  local_stubs_name = NULL;

  return 0;
}

static void rm_tempfile(void)
{
  abort_import();
  if(temp_name)
    unlink(temp_name);
  if (do_header)
    unlink(header_name);
  if (local_stubs_name)
    unlink(local_stubs_name);
  if (do_client)
    unlink(client_name);
  if (do_server)
    unlink(server_name);
  if (do_idfile)
    unlink(idfile_name);
  if (do_proxies)
    unlink(proxy_name);
  if (do_typelib)
    unlink(typelib_name);
}
