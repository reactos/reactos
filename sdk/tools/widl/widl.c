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

static const char usage[] =
"Usage: widl [options...] infile.idl\n"
"   or: widl [options...] --dlldata-only name1 [name2...]\n"
"   --acf=file         Use ACF file\n"
"   -app_config        Ignored, present for midl compatibility\n"
"   -b arch            Set the target architecture\n"
"   -c                 Generate client stub\n"
"   -d n               Set debug level to 'n'\n"
"   -D id[=val]        Define preprocessor identifier id=val\n"
"   -E                 Preprocess only\n"
"   --help             Display this help and exit\n"
"   -h                 Generate headers\n"
"   -H file            Name of header file (default is infile.h)\n"
"   -I path            Set include search dir to path (multiple -I allowed)\n"
"   --local-stubs=file Write empty stubs for call_as/local methods to file\n"
"   -m32, -m64         Set the target architecture (Win32 or Win64)\n"
"   -N                 Do not preprocess input\n"
"   --oldnames         Use old naming conventions\n"
"   --oldtlb           Use old typelib (SLTG) format\n"
"   -o, --output=NAME  Set the output file name\n"
"   -Otype             Type of stubs to generate (-Os, -Oi, -Oif)\n"
"   -p                 Generate proxy\n"
"   --prefix-all=p     Prefix names of client stubs / server functions with 'p'\n"
"   --prefix-client=p  Prefix names of client stubs with 'p'\n"
"   --prefix-server=p  Prefix names of server functions with 'p'\n"
"   -r                 Generate registration script\n"
"   -robust            Ignored, present for midl compatibility\n"
"   --winrt            Enable Windows Runtime mode\n"
"   --ns_prefix        Prefix namespaces with ABI namespace\n"
"   -s                 Generate server stub\n"
"   -t                 Generate typelib\n"
"   -u                 Generate interface identifiers file\n"
"   -V                 Print version and exit\n"
"   -W                 Enable pedantic warnings\n"
"   --win32, --win64   Set the target architecture (Win32 or Win64)\n"
"   --win32-align n    Set win32 structure alignment to 'n'\n"
"   --win64-align n    Set win64 structure alignment to 'n'\n"
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

#ifdef __i386__
enum target_cpu target_cpu = CPU_x86;
#elif defined(__x86_64__)
enum target_cpu target_cpu = CPU_x86_64;
#elif defined(__powerpc__)
enum target_cpu target_cpu = CPU_POWERPC;
#elif defined(__arm__)
enum target_cpu target_cpu = CPU_ARM;
#elif defined(__aarch64__)
enum target_cpu target_cpu = CPU_ARM64;
#else
#error Unsupported CPU
#endif

int debuglevel = DEBUGLEVEL_NONE;
int parser_debug, yy_flex_debug;

int pedantic = 0;
int do_everything = 1;
static int preprocess_only = 0;
int do_header = 0;
int do_typelib = 0;
int do_old_typelib = 0;
int do_proxies = 0;
int do_client = 0;
int do_server = 0;
int do_regscript = 0;
int do_idfile = 0;
int do_dlldata = 0;
static int no_preprocess = 0;
int old_names = 0;
int win32_packing = 8;
int win64_packing = 8;
int winrt_mode = 0;
int use_abi_namespace = 0;
static enum stub_mode stub_mode = MODE_Os;

char *input_name;
char *input_idl_name;
char *acf_name;
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
char *regscript_name;
char *regscript_token;
static char *idfile_name;
char *temp_name;
const char *prefix_client = "";
const char *prefix_server = "";

int line_number = 1;

static FILE *idfile;

unsigned int pointer_size = 0;

time_t now;

enum {
    OLDNAMES_OPTION = CHAR_MAX + 1,
    ACF_OPTION,
    APP_CONFIG_OPTION,
    DLLDATA_OPTION,
    DLLDATA_ONLY_OPTION,
    LOCAL_STUBS_OPTION,
    OLD_TYPELIB_OPTION,
    PREFIX_ALL_OPTION,
    PREFIX_CLIENT_OPTION,
    PREFIX_SERVER_OPTION,
    PRINT_HELP,
    RT_NS_PREFIX,
    RT_OPTION,
    ROBUST_OPTION,
    WIN32_OPTION,
    WIN64_OPTION,
    WIN32_ALIGN_OPTION,
    WIN64_ALIGN_OPTION
};

static const char short_options[] =
    "b:cC:d:D:EhH:I:m:No:O:pP:rsS:tT:uU:VW";
static const struct option long_options[] = {
    { "acf", 1, NULL, ACF_OPTION },
    { "app_config", 0, NULL, APP_CONFIG_OPTION },
    { "dlldata", 1, NULL, DLLDATA_OPTION },
    { "dlldata-only", 0, NULL, DLLDATA_ONLY_OPTION },
    { "help", 0, NULL, PRINT_HELP },
    { "local-stubs", 1, NULL, LOCAL_STUBS_OPTION },
    { "ns_prefix", 0, NULL, RT_NS_PREFIX },
    { "oldnames", 0, NULL, OLDNAMES_OPTION },
    { "oldtlb", 0, NULL, OLD_TYPELIB_OPTION },
    { "output", 0, NULL, 'o' },
    { "prefix-all", 1, NULL, PREFIX_ALL_OPTION },
    { "prefix-client", 1, NULL, PREFIX_CLIENT_OPTION },
    { "prefix-server", 1, NULL, PREFIX_SERVER_OPTION },
    { "robust", 0, NULL, ROBUST_OPTION },
    { "target", 0, NULL, 'b' },
    { "winrt", 0, NULL, RT_OPTION },
    { "win32", 0, NULL, WIN32_OPTION },
    { "win64", 0, NULL, WIN64_OPTION },
    { "win32-align", 1, NULL, WIN32_ALIGN_OPTION },
    { "win64-align", 1, NULL, WIN64_ALIGN_OPTION },
    { NULL, 0, NULL, 0 }
};

static void rm_tempfile(void);

enum stub_mode get_stub_mode(void)
{
    /* old-style interpreted stubs are not supported on 64-bit */
    if (stub_mode == MODE_Oi && pointer_size == 8) return MODE_Oif;
    return stub_mode;
}

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
    else token[i] = tolower(token[i]);
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

static void add_widl_version_define(void)
{
    unsigned int version;
    const char *p = PACKAGE_VERSION;

    /* major */
    version = atoi(p) * 0x10000;
    p = strchr(p, '.');

    /* minor */
    if (p)
    {
        version += atoi(p + 1) * 0x100;
        p = strchr(p + 1, '.');
    }

    /* build */
    if (p)
        version += atoi(p + 1);

    if (version != 0)
    {
        char version_str[11];
        snprintf(version_str, sizeof(version_str), "0x%x", version);
        wpp_add_define("__WIDL__", version_str);
    }
    else
        wpp_add_define("__WIDL__", NULL);
}

/* set the target platform */
static void set_target( const char *target )
{
    static const struct
    {
        const char     *name;
        enum target_cpu cpu;
    } cpu_names[] =
    {
        { "i386",    CPU_x86 },
        { "i486",    CPU_x86 },
        { "i586",    CPU_x86 },
        { "i686",    CPU_x86 },
        { "i786",    CPU_x86 },
        { "amd64",   CPU_x86_64 },
        { "x86_64",  CPU_x86_64 },
        { "powerpc", CPU_POWERPC },
        { "arm",     CPU_ARM },
        { "armv5",   CPU_ARM },
        { "armv6",   CPU_ARM },
        { "armv7",   CPU_ARM },
        { "arm64",   CPU_ARM64 },
        { "aarch64", CPU_ARM64 },
    };

    unsigned int i;
    char *p, *spec = xstrdup( target );

    /* target specification is in the form CPU-MANUFACTURER-OS or CPU-MANUFACTURER-KERNEL-OS */

    if (!(p = strchr( spec, '-' ))) error( "Invalid target specification '%s'\n", target );
    *p++ = 0;
    for (i = 0; i < sizeof(cpu_names)/sizeof(cpu_names[0]); i++)
    {
        if (!strcmp( cpu_names[i].name, spec ))
        {
            target_cpu = cpu_names[i].cpu;
            free( spec );
            return;
        }
    }
    error( "Unrecognized CPU '%s'\n", spec );
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
  do_old_typelib = x;
  do_proxies = x;
  do_client = x;
  do_server = x;
  do_regscript = x;
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

static void write_dlldata_list(struct list *filenames, int define_proxy_delegation)
{
  FILE *dlldata;
  filename_node_t *node;

  dlldata = fopen(dlldata_name, "w");
  if (!dlldata)
    error("couldn't open %s: %s\n", dlldata_name, strerror(errno));

  fprintf(dlldata, "/*** Autogenerated by WIDL %s ", PACKAGE_VERSION);
  fprintf(dlldata, "- Do not edit ***/\n\n");
  if (define_proxy_delegation)
      fprintf(dlldata, "#define PROXY_DELEGATION\n");

  fprintf(dlldata, "#ifdef __REACTOS__\n");
  fprintf(dlldata, "#define WIN32_NO_STATUS\n");
  fprintf(dlldata, "#define WIN32_LEAN_AND_MEAN\n");
  fprintf(dlldata, "#endif\n\n");

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
  int define_proxy_delegation = 0;
  filename_node_t *node;
  FILE *dlldata;

  if (!do_dlldata || !need_proxy_file(stmts))
    return;

  define_proxy_delegation = need_proxy_delegation(stmts);

  dlldata = fopen(dlldata_name, "r");
  if (dlldata) {
    static const char marker[] = "REFERENCE_PROXY_FILE";
    static const char delegation_define[] = "#define PROXY_DELEGATION";
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
      }else if (!define_proxy_delegation && strncmp(start, delegation_define, sizeof(delegation_define)-1)) {
          define_proxy_delegation = 1;
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
  write_dlldata_list(&filenames, define_proxy_delegation);
  free_filename_nodes(&filenames);
}

static void write_id_guid(FILE *f, const char *type, const char *guid_prefix, const char *name, const UUID *uuid)
{
  if (!uuid) return;
  fprintf(f, "MIDL_DEFINE_GUID(%s, %s_%s, 0x%08x, 0x%04x, 0x%04x, 0x%02x,0x%02x, 0x%02x,"
        "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x);\n",
        type, guid_prefix, name, uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0],
        uuid->Data4[1], uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5],
        uuid->Data4[6], uuid->Data4[7]);
}

static void write_id_data_stmts(const statement_list_t *stmts)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    if (stmt->type == STMT_TYPE)
    {
      const type_t *type = stmt->u.type;
      if (type_get_type(type) == TYPE_INTERFACE)
      {
        const UUID *uuid;
        if (!is_object(type) && !is_attr(type->attrs, ATTR_DISPINTERFACE))
          continue;
        uuid = get_attrp(type->attrs, ATTR_UUID);
        write_id_guid(idfile, "IID", is_attr(type->attrs, ATTR_DISPINTERFACE) ? "DIID" : "IID",
                   type->name, uuid);
        if (type->details.iface->async_iface)
        {
          uuid = get_attrp(type->details.iface->async_iface->attrs, ATTR_UUID);
          write_id_guid(idfile, "IID", "IID", type->details.iface->async_iface->name, uuid);
        }
      }
      else if (type_get_type(type) == TYPE_COCLASS)
      {
        const UUID *uuid = get_attrp(type->attrs, ATTR_UUID);
        write_id_guid(idfile, "CLSID", "CLSID", type->name, uuid);
      }
    }
    else if (stmt->type == STMT_LIBRARY)
    {
      const UUID *uuid = get_attrp(stmt->u.lib->attrs, ATTR_UUID);
      write_id_guid(idfile, "IID", "LIBID", stmt->u.lib->name, uuid);
      write_id_data_stmts(stmt->u.lib->stmts);
    }
  }
}

void write_id_data(const statement_list_t *stmts)
{
  if (!do_idfile) return;

  idfile = fopen(idfile_name, "w");
  if (! idfile) {
    error("Could not open %s for output\n", idfile_name);
    return;
  }

  fprintf(idfile, "/*** Autogenerated by WIDL %s ", PACKAGE_VERSION);
  fprintf(idfile, "from %s - Do not edit ***/\n\n", input_idl_name);

  fprintf(idfile, "#ifdef __REACTOS__\n");
  fprintf(idfile, "#define WIN32_NO_STATUS\n");
  fprintf(idfile, "#define WIN32_LEAN_AND_MEAN\n");
  fprintf(idfile, "#endif\n\n");

  fprintf(idfile, "#include <rpc.h>\n");
  fprintf(idfile, "#include <rpcndr.h>\n\n");

  fprintf(idfile, "#ifdef _MIDL_USE_GUIDDEF_\n\n");

  fprintf(idfile, "#ifndef INITGUID\n");
  fprintf(idfile, "#define INITGUID\n");
  fprintf(idfile, "#include <guiddef.h>\n");
  fprintf(idfile, "#undef INITGUID\n");
  fprintf(idfile, "#else\n");
  fprintf(idfile, "#include <guiddef.h>\n");
  fprintf(idfile, "#endif\n\n");

  fprintf(idfile, "#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \\\n");
  fprintf(idfile, "    DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)\n\n");

  fprintf(idfile, "#elif defined(__cplusplus)\n\n");

  fprintf(idfile, "#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \\\n");
  fprintf(idfile, "    EXTERN_C const type DECLSPEC_SELECTANY name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}\n\n");

  fprintf(idfile, "#else\n\n");

  fprintf(idfile, "#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \\\n");
  fprintf(idfile, "    const type DECLSPEC_SELECTANY name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}\n\n");

  fprintf(idfile, "#endif\n\n");
  start_cplusplus_guard(idfile);

  write_id_data_stmts(stmts);

  fprintf(idfile, "\n");
  end_cplusplus_guard(idfile);
  fprintf(idfile, "#undef MIDL_DEFINE_GUID\n" );

  fclose(idfile);
}

int main(int argc,char *argv[])
{
  int optc;
  int ret = 0;
  int opti = 0;
  char *output_name = NULL;

  signal( SIGTERM, exit_on_signal );
  signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
  signal( SIGHUP, exit_on_signal );
#endif

  now = time(NULL);

  while((optc = getopt_long_only(argc, argv, short_options, long_options, &opti)) != EOF) {
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
    case PRINT_HELP:
      fprintf(stderr, "%s", usage);
      return 0;
    case RT_OPTION:
      winrt_mode = 1;
      break;
    case RT_NS_PREFIX:
      use_abi_namespace = 1;
      break;
    case WIN32_OPTION:
      pointer_size = 4;
      break;
    case WIN64_OPTION:
      pointer_size = 8;
      break;
    case WIN32_ALIGN_OPTION:
      win32_packing = strtol(optarg, NULL, 0);
      if(win32_packing != 2 && win32_packing != 4 && win32_packing != 8)
          error("Packing must be one of 2, 4 or 8\n");
      break;
    case WIN64_ALIGN_OPTION:
      win64_packing = strtol(optarg, NULL, 0);
      if(win64_packing != 2 && win64_packing != 4 && win64_packing != 8)
          error("Packing must be one of 2, 4 or 8\n");
      break;
    case ACF_OPTION:
      acf_name = xstrdup(optarg);
      break;
    case APP_CONFIG_OPTION:
      /* widl does not distinguish between app_mode and default mode,
         but we ignore this option for midl compatibility */
      break;
    case ROBUST_OPTION:
        /* FIXME: Support robust option */
        break;
    case 'b':
      set_target( optarg );
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
    case 'm':
      if (!strcmp( optarg, "32" )) pointer_size = 4;
      else if (!strcmp( optarg, "64" )) pointer_size = 8;
      break;
    case 'N':
      no_preprocess = 1;
      break;
    case 'o':
      output_name = xstrdup(optarg);
      break;
    case 'O':
      if (!strcmp( optarg, "s" )) stub_mode = MODE_Os;
      else if (!strcmp( optarg, "i" )) stub_mode = MODE_Oi;
      else if (!strcmp( optarg, "ic" )) stub_mode = MODE_Oif;
      else if (!strcmp( optarg, "if" )) stub_mode = MODE_Oif;
      else if (!strcmp( optarg, "icf" )) stub_mode = MODE_Oif;
      else error( "Invalid argument '-O%s'\n", optarg );
      break;
    case 'p':
      do_everything = 0;
      do_proxies = 1;
      break;
    case 'P':
      proxy_name = xstrdup(optarg);
      break;
    case 'r':
      do_everything = 0;
      do_regscript = 1;
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
    case OLD_TYPELIB_OPTION:
      do_old_typelib = 1;
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

#ifdef DEFAULT_INCLUDE_DIR
  wpp_add_include_path(DEFAULT_INCLUDE_DIR);
#endif

  switch (target_cpu)
  {
  case CPU_x86:
      if (pointer_size == 8) target_cpu = CPU_x86_64;
      else pointer_size = 4;
      break;
  case CPU_x86_64:
      if (pointer_size == 4) target_cpu = CPU_x86;
      else pointer_size = 8;
      break;
  case CPU_ARM64:
      if (pointer_size == 4) error( "Cannot build 32-bit code for this CPU\n" );
      pointer_size = 8;
      break;
  default:
      if (pointer_size == 8) error( "Cannot build 64-bit code for this CPU\n" );
      pointer_size = 4;
      break;
  }

  /* if nothing specified, try to guess output type from the output file name */
  if (output_name && do_everything && !do_header && !do_typelib && !do_proxies &&
      !do_client && !do_server && !do_regscript && !do_idfile && !do_dlldata)
  {
      do_everything = 0;
      if (strendswith( output_name, ".h" )) do_header = 1;
      else if (strendswith( output_name, ".tlb" )) do_typelib = 1;
      else if (strendswith( output_name, "_p.c" )) do_proxies = 1;
      else if (strendswith( output_name, "_c.c" )) do_client = 1;
      else if (strendswith( output_name, "_s.c" )) do_server = 1;
      else if (strendswith( output_name, "_i.c" )) do_idfile = 1;
      else if (strendswith( output_name, "_r.res" )) do_regscript = 1;
      else if (strendswith( output_name, "_t.res" )) do_typelib = 1;
      else if (strendswith( output_name, "dlldata.c" )) do_dlldata = 1;
      else do_everything = 1;
  }

  if(do_everything) {
    set_everything(TRUE);
  }

  if (!output_name) output_name = dup_basename(input_name, ".idl");

  if (do_header + do_typelib + do_proxies + do_client +
      do_server + do_regscript + do_idfile + do_dlldata == 1)
  {
      if (do_header) header_name = output_name;
      else if (do_typelib) typelib_name = output_name;
      else if (do_proxies) proxy_name = output_name;
      else if (do_client) client_name = output_name;
      else if (do_server) server_name = output_name;
      else if (do_regscript) regscript_name = output_name;
      else if (do_idfile) idfile_name = output_name;
      else if (do_dlldata) dlldata_name = output_name;
  }

  if (!dlldata_name && do_dlldata)
    dlldata_name = xstrdup("dlldata.c");

  if(optind < argc) {
    if (do_dlldata && !do_everything) {
      struct list filenames = LIST_INIT(filenames);
      for ( ; optind < argc; ++optind)
        add_filename_node(&filenames, argv[optind]);

      write_dlldata_list(&filenames, 0 /* FIXME */ );
      free_filename_nodes(&filenames);
      return 0;
    }
    else if (optind != argc - 1) {
      fprintf(stderr, "%s", usage);
      return 1;
    }
    else
      input_idl_name = input_name = xstrdup(argv[optind]);
  }
  else {
    fprintf(stderr, "%s", usage);
    return 1;
  }

  if(debuglevel)
  {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
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

  if (!regscript_name && do_regscript) {
    regscript_name = dup_basename(input_name, ".idl");
    strcat(regscript_name, "_r.rgs");
  }

  if (!idfile_name && do_idfile) {
    idfile_name = dup_basename(input_name, ".idl");
    strcat(idfile_name, "_i.c");
  }

  if (do_proxies) proxy_token = dup_basename_token(proxy_name,"_p.c");
  if (do_client) client_token = dup_basename_token(client_name,"_c.c");
  if (do_server) server_token = dup_basename_token(server_name,"_s.c");
  if (do_regscript) regscript_token = dup_basename_token(regscript_name,"_r.rgs");

  add_widl_version_define();
  wpp_add_define("_WIN32", NULL);

  atexit(rm_tempfile);
  if (!no_preprocess)
  {
    chat("Starting preprocess\n");

    if (!preprocess_only)
    {
        FILE *output;
        int fd;
        char *name = xmalloc( strlen(header_name) + 8 );

        strcpy( name, header_name );
        strcat( name, ".XXXXXX" );

        if ((fd = mkstemps( name, 0 )) == -1)
            error("Could not generate a temp name from %s\n", name);

        temp_name = name;
        if (!(output = fdopen(fd, "wt")))
            error("Could not open fd %s for writing\n", name);

        ret = wpp_parse( input_name, output );
        fclose( output );
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
  if (do_regscript)
    unlink(regscript_name);
  if (do_idfile)
    unlink(idfile_name);
  if (do_proxies)
    unlink(proxy_name);
  if (do_typelib)
    unlink(typelib_name);
}
