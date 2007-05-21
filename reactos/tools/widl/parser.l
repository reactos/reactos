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
%option nounput noyy_top_state
%option 8bit never-interactive

nl	\r?\n
ws	[ \f\t\r]
cident	[a-zA-Z_][0-9a-zA-Z_]*
int	[0-9]+
hexd	[0-9a-fA-F]
hex	0x{hexd}+
uuid	{hexd}{8}-{hexd}{4}-{hexd}{4}-{hexd}{4}-{hexd}{12}

%x QUOTE
%x pp_line

%{

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "wine/wpp.h"

#include "parser.tab.h"

extern char *temp_name;

static void addcchar(char c);
static char *get_buffered_cstring(void);

static char *cbuffer;
static int cbufidx;
static int cbufalloc = 0;

static int kw_token(const char *kw);

#define MAX_IMPORT_DEPTH 10
struct {
  YY_BUFFER_STATE state;
  char *input_name;
  int   line_number;
  char *temp_name;
} import_stack[MAX_IMPORT_DEPTH];
int import_stack_ptr = 0;

static void pop_import(void);

static UUID* parse_uuid(const char*u)
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
<INITIAL>^{ws}*\#{ws}*	yy_push_state(pp_line);
<pp_line>[^\n]*         {
                            int lineno;
                            char *cptr, *fname;
                            yy_pop_state();
                            lineno = (int)strtol(yytext, &cptr, 10);
                            if(!lineno)
                                yyerror("Malformed '#...' line-directive; invalid linenumber");
                            fname = strchr(cptr, '"');
                            if(!fname)
                                yyerror("Malformed '#...' line-directive; missing filename");
                            fname++;
                            cptr = strchr(fname, '"');
                            if(!cptr)
                                yyerror("Malformed '#...' line-directive; missing terminating \"");
                            *cptr = '\0';
                            line_number = lineno - 1;  /* We didn't read the newline */
                            free( input_name );
                            input_name = xstrdup(fname);
                        }
\"			yy_push_state(QUOTE); cbufidx = 0;
<QUOTE>\"		{
				yy_pop_state();
				yylval.str = get_buffered_cstring();
				return aSTRING;
			}
<QUOTE>\\\\		|
<QUOTE>\\\"		addcchar(yytext[1]);
<QUOTE>\\.		addcchar('\\'); addcchar(yytext[1]);
<QUOTE>.		addcchar(yytext[0]);
{uuid}			{
				yylval.uuid = parse_uuid(yytext);
				return aUUID;
			}
{hex}			{
				yylval.num = strtoul(yytext, NULL, 0);
				return aHEXNUM;
			}
{int}			{
				yylval.num = strtoul(yytext, NULL, 0);
				return aNUM;
			}
SAFEARRAY{ws}*/\(	return tSAFEARRAY;
{cident}		return kw_token(yytext);
\n			line_number++;
{ws}
\<\<			return SHL;
\>\>			return SHR;
.			return yytext[0];
<<EOF>>			{
				if (import_stack_ptr) {
					pop_import();
					return aEOF;
				}
				else yyterminate();
			}
%%

#ifndef yywrap
int yywrap(void)
{
	return 1;
}
#endif

static struct keyword {
	const char *kw;
	int token;
	int val;
} keywords[] = {
	{"FALSE",			tFALSE},
	{"TRUE",			tTRUE},
	{"__cdecl",			tCDECL},
	{"__int64",			tINT64},
	{"__stdcall",			tSTDCALL},
	{"_stdcall",			tSTDCALL},
	{"aggregatable",		tAGGREGATABLE},
	{"allocate",			tALLOCATE},
	{"appobject",			tAPPOBJECT},
	{"async",			tASYNC},
	{"async_uuid",			tASYNCUUID},
	{"auto_handle",			tAUTOHANDLE},
	{"bindable",			tBINDABLE},
	{"boolean",			tBOOLEAN},
	{"broadcast",			tBROADCAST},
	{"byte",			tBYTE},
	{"byte_count",			tBYTECOUNT},
	{"call_as",			tCALLAS},
	{"callback",			tCALLBACK},
	{"case",			tCASE},
	{"char",			tCHAR},
	{"coclass",			tCOCLASS},
	{"code",			tCODE},
	{"comm_status",			tCOMMSTATUS},
	{"const",			tCONST},
	{"context_handle",		tCONTEXTHANDLE},
	{"context_handle_noserialize",	tCONTEXTHANDLENOSERIALIZE},
	{"context_handle_serialize",	tCONTEXTHANDLENOSERIALIZE},
	{"control",			tCONTROL},
	{"cpp_quote",			tCPPQUOTE},
/* ... */
	{"default",			tDEFAULT},
	{"defaultcollelem",		tDEFAULTCOLLELEM},
	{"defaultvalue",		tDEFAULTVALUE},
	{"defaultvtable",		tDEFAULTVTABLE},
	{"dispinterface",		tDISPINTERFACE},
	{"displaybind",			tDISPLAYBIND},
	{"dllname",			tDLLNAME},
	{"double",			tDOUBLE},
	{"dual",			tDUAL},
	{"endpoint",			tENDPOINT},
	{"entry",			tENTRY},
	{"enum",			tENUM},
	{"error_status_t",		tERRORSTATUST},
	{"explicit_handle",		tEXPLICITHANDLE},
	{"extern",			tEXTERN},
	{"float",			tFLOAT},
	{"handle",			tHANDLE},
	{"handle_t",			tHANDLET},
	{"helpcontext",			tHELPCONTEXT},
	{"helpfile",			tHELPFILE},
	{"helpstring",			tHELPSTRING},
	{"helpstringcontext",		tHELPSTRINGCONTEXT},
	{"helpstringdll",		tHELPSTRINGDLL},
	{"hidden",                      tHIDDEN},
	{"hyper",			tHYPER},
	{"id",				tID},
	{"idempotent",			tIDEMPOTENT},
/* ... */
	{"iid_is",			tIIDIS},
	{"immediatebind",		tIMMEDIATEBIND},
	{"implicit_handle",		tIMPLICITHANDLE},
	{"import",			tIMPORT},
	{"importlib",			tIMPORTLIB},
	{"in",				tIN},
	{"in_line",			tINLINE},
	{"input_sync",			tINPUTSYNC},
	{"int",				tINT},
/* ... */
	{"interface",			tINTERFACE},
	{"lcid",			tLCID},
	{"length_is",			tLENGTHIS},
	{"library",			tLIBRARY},
/* ... */
	{"local",			tLOCAL},
	{"long",			tLONG},
/* ... */
	{"methods",			tMETHODS},
/* ... */
	{"module",			tMODULE},
/* ... */
	{"nonbrowsable",		tNONBROWSABLE},
	{"noncreatable",		tNONCREATABLE},
	{"nonextensible",		tNONEXTENSIBLE},
	{"object",			tOBJECT},
	{"odl",				tODL},
	{"oleautomation",		tOLEAUTOMATION},
/* ... */
	{"optional",			tOPTIONAL},
	{"out",				tOUT},
/* ... */
	{"pointer_default",		tPOINTERDEFAULT},
/* ... */
	{"properties",			tPROPERTIES},
	{"propget",			tPROPGET},
	{"propput",			tPROPPUT},
	{"propputref",			tPROPPUTREF},
        {"ptr",				tPTR},
/* ... */
	{"public",			tPUBLIC},
        {"range",			tRANGE},
/* ... */
	{"readonly",			tREADONLY},
	{"ref",				tREF},
	{"requestedit",			tREQUESTEDIT},
	{"restricted",                  tRESTRICTED},
	{"retval",			tRETVAL},
/* ... */
	{"short",			tSHORT},
	{"signed",			tSIGNED},
	{"single",			tSINGLE},
	{"size_is",			tSIZEIS},
	{"sizeof",			tSIZEOF},
        {"small",			tSMALL},
/* ... */
	{"source",			tSOURCE},
/* ... */	
	{"string",			tSTRING},
	{"struct",			tSTRUCT},
	{"switch",			tSWITCH},
	{"switch_is",			tSWITCHIS},
	{"switch_type",			tSWITCHTYPE},
/* ... */
	{"transmit_as",			tTRANSMITAS},
	{"typedef",			tTYPEDEF},
	{"union",			tUNION},
/* ... */
	{"unique",			tUNIQUE},
	{"unsigned",			tUNSIGNED},
/* ... */
	{"uuid",			tUUID},
	{"v1_enum",			tV1ENUM},
/* ... */
	{"vararg",			tVARARG},
	{"version",			tVERSION},
	{"void",			tVOID},
	{"wchar_t",			tWCHAR},
	{"wire_marshal",		tWIREMARSHAL},
};
#define NKEYWORDS (sizeof(keywords)/sizeof(keywords[0]))
#define KWP(p) ((const struct keyword *)(p))

static int kw_cmp_func(const void *s1, const void *s2)
{
	return strcmp(KWP(s1)->kw, KWP(s2)->kw);
}

#define KW_BSEARCH
static int kw_token(const char *kw)
{
	struct keyword key, *kwp;
	key.kw = kw;
#ifdef KW_BSEARCH
	kwp = bsearch(&key, keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
#else
	{
		int i;
		for (kwp=NULL, i=0; i < NKEYWORDS; i++)
			if (!kw_cmp_func(&key, &keywords[i])) {
				kwp = &keywords[i];
				break;
			}
	}
#endif
	if (kwp) {
		yylval.str = (char*)kwp->kw;
		return kwp->token;
	}
	yylval.str = xstrdup(kw);
	return is_type(kw) ? aKNOWNTYPE : aIDENTIFIER;
}

static void addcchar(char c)
{
	if(cbufidx >= cbufalloc)
	{
		cbufalloc += 1024;
		cbuffer = xrealloc(cbuffer, cbufalloc * sizeof(cbuffer[0]));
		if(cbufalloc > 65536)
			yywarning("Reallocating string buffer larger than 64kB");
	}
	cbuffer[cbufidx++] = c;
}

static char *get_buffered_cstring(void)
{
	addcchar(0);
	return xstrdup(cbuffer);
}

static void pop_import(void)
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
	free( input_name );
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
	char *hname, *path, *p;
	struct imports *import;
	int ptr = import_stack_ptr;
	int ret;

	if (!parse_only && do_header) {
		hname = dup_basename(fname, ".idl");
                p = hname + strlen(hname) - 2;
                if (p <= hname || strcmp( p, ".h" )) strcat(hname, ".h");

		fprintf(header, "#include <%s>\n", hname);
		free(hname);
	}

	import = first_import;
	while (import && strcmp(import->name, fname))
		import = import->next;
	if (import) return 0; /* already imported */

	import = xmalloc(sizeof(struct imports));
	import->name = xstrdup(fname);
	import->next = first_import;
	first_import = import;

        if (!(path = wpp_find_include( fname, input_name )))
            yyerror("Unable to open include file %s", fname);

	import_stack[ptr].temp_name = temp_name;
	import_stack[ptr].input_name = input_name;
	import_stack[ptr].line_number = line_number;
	import_stack_ptr++;
        input_name = path;
        line_number = 1;

        ret = wpp_parse_temp( path, NULL, &temp_name );
        if (ret) exit(1);

	if((f = fopen(temp_name, "r")) == NULL)
		yyerror("Unable to open %s", temp_name);

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
