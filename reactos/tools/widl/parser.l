/* -*-C-*-
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

%option stack
%option noinput nounput noyy_top_state
%option 8bit never-interactive prefix="parser_"

nl	\r?\n
ws	[ \f\t\r]
cident	[a-zA-Z_][0-9a-zA-Z_]*
u_suffix	(u|U)
l_suffix	(l|L)
int	[0-9]+({l_suffix}?{u_suffix}?|{u_suffix}?{l_suffix}?)?
hexd	[0-9a-fA-F]
hex	0(x|X){hexd}+({l_suffix}?{u_suffix}?|{u_suffix}?{l_suffix}?)?
uuid	{hexd}{8}-{hexd}{4}-{hexd}{4}-{hexd}{4}-{hexd}{12}
double	[0-9]+\.[0-9]+([eE][+-]?[0-9]+)*

%x QUOTE
%x WSTRQUOTE
%x ATTR
%x PP_LINE
%x PP_PRAGMA
%x SQUOTE

%{

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#define YY_NO_UNISTD_H
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "wine/wpp.h"

#include "parser.tab.h"

static void addcchar(char c);
static char *get_buffered_cstring(void);

static char *cbuffer;
static int cbufidx;
static int cbufalloc = 0;

static int kw_token(const char *kw);
static int attr_token(const char *kw);

#define MAX_IMPORT_DEPTH 10
struct {
  YY_BUFFER_STATE state;
  char *input_name;
  int   line_number;
  char *temp_name;
} import_stack[MAX_IMPORT_DEPTH];
int import_stack_ptr = 0;

/* converts an integer in string form to an unsigned long and prints an error
 * on overflow */
static unsigned int xstrtoul(const char *nptr, char **endptr, int base)
{
    unsigned long val;

    errno = 0;
    val = strtoul(nptr, endptr, base);
    if ((val == ULONG_MAX && errno == ERANGE) || ((unsigned int)val != val))
        error_loc("integer constant %s is too large\n", nptr);
    return val;
}

UUID *parse_uuid(const char *u)
{
  UUID* uuid = xmalloc(sizeof(UUID));
  char b[3];
  /* it would be nice to use UuidFromStringA */
  uuid->Data1 = strtoul(u, NULL, 16);
  uuid->Data2 = strtoul(u+9, NULL, 16);
  uuid->Data3 = strtoul(u+14, NULL, 16);
  b[2] = 0;
  memcpy(b, u+19, 2); uuid->Data4[0] = strtoul(b, NULL, 16);
  memcpy(b, u+21, 2); uuid->Data4[1] = strtoul(b, NULL, 16);
  memcpy(b, u+24, 2); uuid->Data4[2] = strtoul(b, NULL, 16);
  memcpy(b, u+26, 2); uuid->Data4[3] = strtoul(b, NULL, 16);
  memcpy(b, u+28, 2); uuid->Data4[4] = strtoul(b, NULL, 16);
  memcpy(b, u+30, 2); uuid->Data4[5] = strtoul(b, NULL, 16);
  memcpy(b, u+32, 2); uuid->Data4[6] = strtoul(b, NULL, 16);
  memcpy(b, u+34, 2); uuid->Data4[7] = strtoul(b, NULL, 16);
  return uuid;
}

%}

/*
 **************************************************************************
 * The flexer starts here
 **************************************************************************
 */
%%
<INITIAL>^{ws}*\#{ws}*pragma{ws}+ yy_push_state(PP_PRAGMA);
<INITIAL,ATTR>^{ws}*\#{ws}*	yy_push_state(PP_LINE);
<PP_LINE>[^\n]*         {
                            int lineno;
                            char *cptr, *fname;
                            yy_pop_state();
                            lineno = (int)strtol(yytext, &cptr, 10);
                            if(!lineno)
                                error_loc("Malformed '#...' line-directive; invalid linenumber\n");
                            fname = strchr(cptr, '"');
                            if(!fname)
                                error_loc("Malformed '#...' line-directive; missing filename\n");
                            fname++;
                            cptr = strchr(fname, '"');
                            if(!cptr)
                                error_loc("Malformed '#...' line-directive; missing terminating \"\n");
                            *cptr = '\0';
                            line_number = lineno - 1;  /* We didn't read the newline */
                            input_name = xstrdup(fname);
                        }
<PP_PRAGMA>midl_echo[^\n]*  yyless(9); yy_pop_state(); return tCPPQUOTE;
<PP_PRAGMA>winrt[^\n]*  {
                            if(import_stack_ptr) {
                                if(!winrt_mode)
                                    error_loc("winrt IDL file imported in non-winrt mode\n");
                            }else {
                                const char *ptr = yytext+5;

                                winrt_mode = TRUE;

                                while(isspace(*ptr))
                                    ptr++;
                                if(!strncmp(ptr, "ns_prefix", 9) && (!*(ptr += 9) || isspace(*ptr)))
                                    use_abi_namespace = TRUE;
                            }
                            yy_pop_state();
                        }
<PP_PRAGMA>[^\n]*       parser_lval.str = xstrdup(yytext); yy_pop_state(); return aPRAGMA;
<INITIAL,ATTR>\"	yy_push_state(QUOTE); cbufidx = 0;
<QUOTE>\"		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return aSTRING;
			}
<INITIAL,ATTR>L\"	yy_push_state(WSTRQUOTE); cbufidx = 0;
<WSTRQUOTE>\"		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return aWSTRING;
			}
<INITIAL,ATTR>\'	yy_push_state(SQUOTE); cbufidx = 0;
<SQUOTE>\'		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return aSQSTRING;
			}
<QUOTE,WSTRQUOTE,SQUOTE>\\\\	|
<QUOTE,WSTRQUOTE>\\\"	addcchar(yytext[1]);
<SQUOTE>\\\'	addcchar(yytext[1]);
<QUOTE,WSTRQUOTE,SQUOTE>\\.	addcchar('\\'); addcchar(yytext[1]);
<QUOTE,WSTRQUOTE,SQUOTE>.	addcchar(yytext[0]);
<INITIAL,ATTR>\[	yy_push_state(ATTR); return '[';
<ATTR>\]		yy_pop_state(); return ']';
<ATTR>{cident}		return attr_token(yytext);
<ATTR>{uuid}			{
				parser_lval.uuid = parse_uuid(yytext);
				return aUUID;
			}
<INITIAL,ATTR>{hex}	{
				parser_lval.num = xstrtoul(yytext, NULL, 0);
				return aHEXNUM;
			}
<INITIAL,ATTR>{int}	{
				parser_lval.num = xstrtoul(yytext, NULL, 0);
				return aNUM;
			}
<INITIAL>{double}	{
				parser_lval.dbl = strtod(yytext, NULL);
				return aDOUBLE;
			}
SAFEARRAY{ws}*/\(	return tSAFEARRAY;
{cident}		return kw_token(yytext);
<INITIAL,ATTR>\n	line_number++;
<INITIAL,ATTR>{ws}
<INITIAL,ATTR>\<\<	return SHL;
<INITIAL,ATTR>\>\>	return SHR;
<INITIAL,ATTR>\-\>	return MEMBERPTR;
<INITIAL,ATTR>==	return EQUALITY;
<INITIAL,ATTR>!=	return INEQUALITY;
<INITIAL,ATTR>\>=	return GREATEREQUAL;
<INITIAL,ATTR>\<=	return LESSEQUAL;
<INITIAL,ATTR>\|\|	return LOGICALOR;
<INITIAL,ATTR>&&	return LOGICALAND;
<INITIAL,ATTR>\.\.\.	return ELLIPSIS;
<INITIAL,ATTR>.		return yytext[0];
<<EOF>>			{
				if (import_stack_ptr)
					return aEOF;
				else yyterminate();
			}
%%

#ifndef parser_wrap
int parser_wrap(void)
{
	return 1;
}
#endif

struct keyword {
	const char *kw;
	int token;
};

/* This table MUST be alphabetically sorted on the kw field */
static const struct keyword keywords[] = {
	{"FALSE",			tFALSE},
	{"NULL",			tNULL},
	{"TRUE",			tTRUE},
	{"__cdecl",			tCDECL},
	{"__fastcall",			tFASTCALL},
	{"__int3264",			tINT3264},
	{"__int64",			tINT64},
	{"__pascal",			tPASCAL},
	{"__stdcall",			tSTDCALL},
	{"_cdecl",			tCDECL},
	{"_fastcall",			tFASTCALL},
	{"_pascal",			tPASCAL},
	{"_stdcall",			tSTDCALL},
	{"boolean",			tBOOLEAN},
	{"byte",			tBYTE},
	{"case",			tCASE},
	{"cdecl",			tCDECL},
	{"char",			tCHAR},
	{"coclass",			tCOCLASS},
	{"const",			tCONST},
	{"cpp_quote",			tCPPQUOTE},
	{"default",			tDEFAULT},
	{"dispinterface",		tDISPINTERFACE},
	{"double",			tDOUBLE},
	{"enum",			tENUM},
	{"error_status_t",		tERRORSTATUST},
	{"extern",			tEXTERN},
	{"float",			tFLOAT},
	{"handle_t",			tHANDLET},
	{"hyper",			tHYPER},
	{"import",			tIMPORT},
	{"importlib",			tIMPORTLIB},
	{"inline",			tINLINE},
	{"int",				tINT},
	{"interface",			tINTERFACE},
	{"library",			tLIBRARY},
	{"long",			tLONG},
	{"methods",			tMETHODS},
	{"module",			tMODULE},
	{"namespace",			tNAMESPACE},
	{"pascal",			tPASCAL},
	{"properties",			tPROPERTIES},
	{"register",			tREGISTER},
	{"short",			tSHORT},
	{"signed",			tSIGNED},
	{"sizeof",			tSIZEOF},
        {"small",			tSMALL},
	{"static",			tSTATIC},
	{"stdcall",			tSTDCALL},
	{"struct",			tSTRUCT},
	{"switch",			tSWITCH},
	{"typedef",			tTYPEDEF},
	{"union",			tUNION},
	{"unsigned",			tUNSIGNED},
	{"void",			tVOID},
	{"wchar_t",			tWCHAR},
};
#define NKEYWORDS (sizeof(keywords)/sizeof(keywords[0]))

/* keywords only recognized in attribute lists
 * This table MUST be alphabetically sorted on the kw field
 */
static const struct keyword attr_keywords[] =
{
        {"aggregatable",                tAGGREGATABLE},
        {"allocate",                    tALLOCATE},
        {"annotation",                  tANNOTATION},
        {"apartment",                   tAPARTMENT},
        {"appobject",                   tAPPOBJECT},
        {"async",                       tASYNC},
        {"async_uuid",                  tASYNCUUID},
        {"auto_handle",                 tAUTOHANDLE},
        {"bindable",                    tBINDABLE},
        {"both",                        tBOTH},
        {"broadcast",                   tBROADCAST},
        {"byte_count",                  tBYTECOUNT},
        {"call_as",                     tCALLAS},
        {"callback",                    tCALLBACK},
        {"code",                        tCODE},
        {"comm_status",                 tCOMMSTATUS},
        {"context_handle",              tCONTEXTHANDLE},
        {"context_handle_noserialize",  tCONTEXTHANDLENOSERIALIZE},
        {"context_handle_serialize",    tCONTEXTHANDLENOSERIALIZE},
        {"control",                     tCONTROL},
        {"decode",                      tDECODE},
        {"defaultbind",                 tDEFAULTBIND},
        {"defaultcollelem",             tDEFAULTCOLLELEM},
        {"defaultvalue",                tDEFAULTVALUE},
        {"defaultvtable",               tDEFAULTVTABLE},
        {"disable_consistency_check",   tDISABLECONSISTENCYCHECK},
        {"displaybind",                 tDISPLAYBIND},
        {"dllname",                     tDLLNAME},
        {"dual",                        tDUAL},
        {"enable_allocate",             tENABLEALLOCATE},
        {"encode",                      tENCODE},
        {"endpoint",                    tENDPOINT},
        {"entry",                       tENTRY},
        {"explicit_handle",             tEXPLICITHANDLE},
        {"fault_status",                tFAULTSTATUS},
        {"force_allocate",              tFORCEALLOCATE},
        {"free",                        tFREE},
        {"handle",                      tHANDLE},
        {"helpcontext",                 tHELPCONTEXT},
        {"helpfile",                    tHELPFILE},
        {"helpstring",                  tHELPSTRING},
        {"helpstringcontext",           tHELPSTRINGCONTEXT},
        {"helpstringdll",               tHELPSTRINGDLL},
        {"hidden",                      tHIDDEN},
        {"id",                          tID},
        {"idempotent",                  tIDEMPOTENT},
        {"ignore",                      tIGNORE},
        {"iid_is",                      tIIDIS},
        {"immediatebind",               tIMMEDIATEBIND},
        {"implicit_handle",             tIMPLICITHANDLE},
        {"in",                          tIN},
        {"in_line",                     tIN_LINE},
        {"input_sync",                  tINPUTSYNC},
        {"lcid",                        tLCID},
        {"length_is",                   tLENGTHIS},
        {"licensed",                    tLICENSED},
        {"local",                       tLOCAL},
        {"maybe",                       tMAYBE},
        {"message",                     tMESSAGE},
        {"neutral",                     tNEUTRAL},
        {"nocode",                      tNOCODE},
        {"nonbrowsable",                tNONBROWSABLE},
        {"noncreatable",                tNONCREATABLE},
        {"nonextensible",               tNONEXTENSIBLE},
        {"notify",                      tNOTIFY},
        {"notify_flag",                 tNOTIFYFLAG},
        {"object",                      tOBJECT},
        {"odl",                         tODL},
        {"oleautomation",               tOLEAUTOMATION},
        {"optimize",                    tOPTIMIZE},
        {"optional",                    tOPTIONAL},
        {"out",                         tOUT},
        {"partial_ignore",              tPARTIALIGNORE},
        {"pointer_default",             tPOINTERDEFAULT},
        {"progid",                      tPROGID},
        {"propget",                     tPROPGET},
        {"propput",                     tPROPPUT},
        {"propputref",                  tPROPPUTREF},
        {"proxy",                       tPROXY},
        {"ptr",                         tPTR},
        {"public",                      tPUBLIC},
        {"range",                       tRANGE},
        {"readonly",                    tREADONLY},
        {"ref",                         tREF},
        {"represent_as",                tREPRESENTAS},
        {"requestedit",                 tREQUESTEDIT},
        {"restricted",                  tRESTRICTED},
        {"retval",                      tRETVAL},
        {"single",                      tSINGLE},
        {"size_is",                     tSIZEIS},
        {"source",                      tSOURCE},
        {"strict_context_handle",       tSTRICTCONTEXTHANDLE},
        {"string",                      tSTRING},
        {"switch_is",                   tSWITCHIS},
        {"switch_type",                 tSWITCHTYPE},
        {"threading",                   tTHREADING},
        {"transmit_as",                 tTRANSMITAS},
        {"uidefault",                   tUIDEFAULT},
        {"unique",                      tUNIQUE},
        {"user_marshal",                tUSERMARSHAL},
        {"usesgetlasterror",            tUSESGETLASTERROR},
        {"uuid",                        tUUID},
        {"v1_enum",                     tV1ENUM},
        {"vararg",                      tVARARG},
        {"version",                     tVERSION},
        {"vi_progid",                   tVIPROGID},
        {"wire_marshal",                tWIREMARSHAL},
};

/* attributes TODO:
    custom
    first_is
    last_is
    max_is
    min_is
*/

#define KWP(p) ((const struct keyword *)(p))

static int kw_cmp_func(const void *s1, const void *s2)
{
	return strcmp(KWP(s1)->kw, KWP(s2)->kw);
}

static int kw_token(const char *kw)
{
	struct keyword key, *kwp;
	key.kw = kw;
	kwp = bsearch(&key, keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
	if (kwp && (winrt_mode || kwp->token != tNAMESPACE)) {
		parser_lval.str = xstrdup(kwp->kw);
		return kwp->token;
	}
	parser_lval.str = xstrdup(kw);
	return is_type(kw) ? aKNOWNTYPE : aIDENTIFIER;
}

static int attr_token(const char *kw)
{
        struct keyword key, *kwp;
        key.kw = kw;
        kwp = bsearch(&key, attr_keywords, sizeof(attr_keywords)/sizeof(attr_keywords[0]),
                      sizeof(attr_keywords[0]), kw_cmp_func);
        if (kwp) {
            parser_lval.str = xstrdup(kwp->kw);
            return kwp->token;
        }
        return kw_token(kw);
}

static void addcchar(char c)
{
	if(cbufidx >= cbufalloc)
	{
		cbufalloc += 1024;
		cbuffer = xrealloc(cbuffer, cbufalloc * sizeof(cbuffer[0]));
		if(cbufalloc > 65536)
			parser_warning("Reallocating string buffer larger than 64kB\n");
	}
	cbuffer[cbufidx++] = c;
}

static char *get_buffered_cstring(void)
{
	addcchar(0);
	return xstrdup(cbuffer);
}

void pop_import(void)
{
	int ptr = import_stack_ptr-1;

	fclose(yyin);
	yy_delete_buffer( YY_CURRENT_BUFFER );
	yy_switch_to_buffer( import_stack[ptr].state );
	if (temp_name) {
		unlink(temp_name);
		free(temp_name);
	}
	temp_name = import_stack[ptr].temp_name;
	input_name = import_stack[ptr].input_name;
	line_number = import_stack[ptr].line_number;
	import_stack_ptr--;
}

struct imports {
	char *name;
	struct imports *next;
} *first_import;

int do_import(char *fname)
{
	FILE *f;
	char *path, *name;
	struct imports *import;
	int ptr = import_stack_ptr;
	int ret, fd;

	import = first_import;
	while (import && strcmp(import->name, fname))
		import = import->next;
	if (import) return 0; /* already imported */

	import = xmalloc(sizeof(struct imports));
	import->name = xstrdup(fname);
	import->next = first_import;
	first_import = import;

        /* don't search for a file name with a path in the include directories,
         * for compatibility with MIDL */
        if (strchr( fname, '/' ) || strchr( fname, '\\' ))
            path = xstrdup( fname );
        else if (!(path = wpp_find_include( fname, input_name )))
            error_loc("Unable to open include file %s\n", fname);

	import_stack[ptr].temp_name = temp_name;
	import_stack[ptr].input_name = input_name;
	import_stack[ptr].line_number = line_number;
	import_stack_ptr++;
        input_name = path;
        line_number = 1;

        name = xstrdup( "widl.XXXXXX" );
        if((fd = mkstemps( name, 0 )) == -1)
            error("Could not generate a temp name from %s\n", name);

        temp_name = name;
        if (!(f = fdopen(fd, "wt")))
            error("Could not open fd %s for writing\n", name);

        ret = wpp_parse( path, f );
        fclose( f );
        if (ret) exit(1);

	if((f = fopen(temp_name, "r")) == NULL)
		error_loc("Unable to open %s\n", temp_name);

	import_stack[ptr].state = YY_CURRENT_BUFFER;
	yy_switch_to_buffer(yy_create_buffer(f, YY_BUF_SIZE));
	return 1;
}

void abort_import(void)
{
	int ptr;

	for (ptr=0; ptr<import_stack_ptr; ptr++)
		unlink(import_stack[ptr].temp_name);
}
