/* -*-C-*-
 * Wrc preprocessor lexical analysis
 *
 * Copyright 1999-2000	Bertho A. Stultiens (BS)
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
 *
 *-------------------------------------------------------------------------
 * The preprocessor's lexographical grammar (approximately):
 *
 * pp		:= {ws} # {ws} if {ws} {expr} {ws} \n
 *		|  {ws} # {ws} ifdef {ws} {id} {ws} \n
 *		|  {ws} # {ws} ifndef {ws} {id} {ws} \n
 *		|  {ws} # {ws} elif {ws} {expr} {ws} \n
 *		|  {ws} # {ws} else {ws} \n
 *		|  {ws} # {ws} endif {ws} \n
 *		|  {ws} # {ws} include {ws} < {anytext} > \n
 *		|  {ws} # {ws} include {ws} " {anytext} " \n
 *		|  {ws} # {ws} define {ws} {anytext} \n
 *		|  {ws} # {ws} define( {arglist} ) {ws} {expansion} \n
 *		|  {ws} # {ws} pragma {ws} {anytext} \n
 *		|  {ws} # {ws} ident {ws} {anytext} \n
 *		|  {ws} # {ws} error {ws} {anytext} \n
 *		|  {ws} # {ws} warning {ws} {anytext} \n
 *		|  {ws} # {ws} line {ws} " {anytext} " {number} \n
 *		|  {ws} # {ws} {number} " {anytext} " {number} [ {number} [{number}] ] \n
 *		|  {ws} # {ws} \n
 *
 * ws		:= [ \t\r\f\v]*
 *
 * expr		:= {expr} [+-*%^/|&] {expr}
 *		|  {expr} {logor|logand} {expr}
 *		|  [!~+-] {expr}
 *		|  {expr} ? {expr} : {expr}
 *
 * logor	:= ||
 *
 * logand	:= &&
 *
 * id		:= [a-zA-Z_][a-zA-Z0-9_]*
 *
 * anytext	:= [^\n]*	(see note)
 *
 * arglist	:=
 *		|  {id}
 *		|  {arglist} , {id}
 *		|  {arglist} , {id} ...
 *
 * expansion	:= {id}
 *		|  # {id}
 *		|  {anytext}
 *		|  {anytext} ## {anytext}
 *
 * number	:= [0-9]+
 *
 * Note: "anytext" is not always "[^\n]*". This is because the
 *	 trailing context must be considered as well.
 *
 * The only certain assumption for the preprocessor to make is that
 * directives start at the beginning of the line, followed by a '#'
 * and end with a newline.
 * Any directive may be suffixed with a line-continuation. Also
 * classical comment / *...* / (note: no comments within comments,
 * therefore spaces) is considered to be a line-continuation
 * (according to gcc and egcs AFAIK, ANSI is a bit vague).
 * Comments have not been added to the above grammar for simplicity
 * reasons. However, it is allowed to enter comment anywhere within
 * the directives as long as they do not interfere with the context.
 * All comments are considered to be deletable whitespace (both
 * classical form "/ *...* /" and C++ form "//...\n").
 *
 * All recursive scans, except for macro-expansion, are done by the
 * parser, whereas the simple state transitions of non-recursive
 * directives are done in the scanner. This results in the many
 * exclusive start-conditions of the scanner.
 *
 * Macro expansions are slightly more difficult because they have to
 * prescan the arguments. Parameter substitution is literal if the
 * substitution is # or ## (either side). This enables new identifiers
 * to be created (see 'info cpp' node Macro|Pitfalls|Prescan for more
 * information).
 *
 * FIXME: Variable macro parameters is recognized, but not yet
 * expanded. I have to reread the ANSI standard on the subject (yes,
 * ANSI defines it).
 *
 * The following special defines are supported:
 * __FILE__	-> "thissource.c"
 * __LINE__	-> 123
 * __DATE__	-> "May  1 2000"
 * __TIME__	-> "23:59:59"
 * These macros expand, as expected, into their ANSI defined values.
 *
 * The same include prevention is implemented as gcc and egcs does.
 * This results in faster processing because we do not read the text
 * at all. Some wine-sources attempt to include the same file 4 or 5
 * times. This strategy also saves a lot blank output-lines, which in
 * its turn improves the real resource scanner/parser.
 *
 */

%top{
#include "config.h"
}

/*
 * Special flex options and exclusive scanner start-conditions
 */
%option stack
%option 8bit never-interactive
%option noinput nounput
%option prefix="ppy_"

%x pp_pp
%x pp_eol
%x pp_inc
%x pp_dqs
%x pp_sqs
%x pp_iqs
%x pp_comment
%x pp_def
%x pp_define
%x pp_macro
%x pp_mbody
%x pp_macign
%x pp_macscan
%x pp_macexp
%x pp_if
%x pp_ifd
%x pp_ifignored
%x pp_endif
%x pp_line
%x pp_defined
%x pp_ignore
%x RCINCL

ws	[ \v\f\t\r]
cident	[a-zA-Z_][0-9a-zA-Z_]*
ul	[uUlL]|[uUlL][lL]|[lL][uU]|[lL][lL][uU]|[uU][lL][lL]|[lL][uU][lL]

%{
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#ifndef LLONG_MAX
# define LLONG_MAX  ((__int64)0x7fffffff << 32 | 0xffffffff)
# define LLONG_MIN  (-LLONG_MAX - 1)
#endif
#ifndef ULLONG_MAX
# define ULLONG_MAX ((__int64)0xffffffff << 32 | 0xffffffff)
#endif

#define YY_NO_UNISTD_H

#include "../tools.h"
#include "utils.h"
#include "wpp_private.h"
#include "ppy.tab.h"

/*
 * Make sure that we are running an appropriate version of flex.
 */
#if !defined(YY_FLEX_MAJOR_VERSION) || (1000 * YY_FLEX_MAJOR_VERSION + YY_FLEX_MINOR_VERSION < 2005)
#error Must use flex version 2.5.1 or higher (yy_scan_* routines are required).
#endif

#define YY_READ_BUF_SIZE	65536		/* So we read most of a file at once */

#define yy_current_state()	YY_START
#define yy_pp_state(x)		yy_pop_state(); yy_push_state(x)

/*
 * Always update the current character position within a line
 */
#define YY_USER_ACTION	pp_status.char_number+=ppy_leng;

/*
 * Buffer management for includes and expansions
 */
#define MAXBUFFERSTACK	128	/* Nesting more than 128 includes or macro expansion textss is insane */

typedef struct bufferstackentry {
	YY_BUFFER_STATE	bufferstate;	/* Buffer to switch back to */
	FILE		*file;          /* File handle */
	pp_entry_t	*define;	/* Points to expanding define or NULL if handling includes */
	int		line_number;	/* Line that we were handling */
	int		char_number;	/* The current position on that line */
	char		*filename;	/* Filename that we were handling */
	int		if_depth;	/* How many #if:s deep to check matching #endif:s */
	int		ncontinuations;	/* Remember the continuation state */
	int		should_pop;	/* Set if we must pop the start-state on EOF */
	/* Include management */
        include_state_t incl;
	char 		*include_filename;
} bufferstackentry_t;

#define ALLOCBLOCKSIZE	(1 << 10)	/* Allocate these chunks at a time for string-buffers */

/*
 * Macro expansion nesting
 * We need the stack to handle expansions while scanning
 * a macro's arguments. The TOS must always be the macro
 * that receives the current expansion from the scanner.
 */
#define MAXMACEXPSTACK	128	/* Nesting more than 128 macro expansions is insane */

typedef struct macexpstackentry {
	pp_entry_t	*ppp;		/* This macro we are scanning */
	char		**args;		/* With these arguments */
	char		**ppargs;	/* Resulting in these preprocessed arguments */
	int		*nnls;		/* Number of newlines per argument */
	int		nargs;		/* And this many arguments scanned */
	int		parentheses;	/* Nesting level of () */
	int		curargsize;	/* Current scanning argument's size */
	int		curargalloc;	/* Current scanning argument's block allocated */
	char		*curarg;	/* Current scanning argument's content */
} macexpstackentry_t;

#define MACROPARENTHESES()	(top_macro()->parentheses)

/*
 * Prototypes
 */
static void newline(int);
static int make_number(int radix, PPY_STYPE *val, const char *str, int len);
static void put_buffer(const char *s, int len);
/* Buffer management */
static void push_buffer(pp_entry_t *ppp, char *filename, char *incname, int pop);
static bufferstackentry_t *pop_buffer(void);
/* String functions */
static void new_string(void);
static void add_string(const char *str, int len);
static char *get_string(void);
static void put_string(void);
static int string_start(void);
/* Macro functions */
static void push_macro(pp_entry_t *ppp);
static macexpstackentry_t *top_macro(void);
static macexpstackentry_t *pop_macro(void);
static void free_macro(macexpstackentry_t *mep);
static void add_text_to_macro(const char *text, int len);
static void macro_add_arg(int last);
static void macro_add_expansion(void);
/* Expansion */
static void expand_special(pp_entry_t *ppp);
static void expand_define(pp_entry_t *ppp);
static void expand_macro(macexpstackentry_t *mep);

/*
 * Local variables
 */
static int ncontinuations;

static int strbuf_idx = 0;
static int strbuf_alloc = 0;
static char *strbuffer = NULL;
static int str_startline;

static macexpstackentry_t *macexpstack[MAXMACEXPSTACK];
static int macexpstackidx = 0;

static bufferstackentry_t bufferstack[MAXBUFFERSTACK];
static int bufferstackidx = 0;

/*
 * Global variables
 */
include_state_t pp_incl_state =
{
    -1,    /* state */
    NULL,  /* ppp */
    0,     /* ifdepth */
    0      /* seen_junk */
};

static struct list pp_includelogiclist = LIST_INIT( pp_includelogiclist );

#define YY_INPUT(buf,result,max_size)					     \
	{								     \
		result = fread(buf, 1, max_size, pp_status.file);	     \
	}

%}

/*
 **************************************************************************
 * The scanner starts here
 **************************************************************************
 */

%%
	/*
	 * Catch line-continuations.
	 * Note: Gcc keeps the line-continuations in, for example, strings
	 * intact. However, I prefer to remove them all so that the next
	 * scanner will not need to reduce the continuation state.
	 *
	 * <*>\\\n		newline(0);
	 */

	/*
	 * Detect the leading # of a preprocessor directive.
	 */
<INITIAL,pp_ignore>^{ws}*#	pp_incl_state.seen_junk++; yy_push_state(pp_pp);

	/*
	 * Scan for the preprocessor directives
	 */
<pp_pp>{ws}*include{ws}*	if(yy_top_state() != pp_ignore) {yy_pp_state(pp_inc); return tINCLUDE;} else {yy_pp_state(pp_eol);}
<pp_pp>{ws}*define{ws}*		yy_pp_state(yy_current_state() != pp_ignore ? pp_def : pp_eol);
<pp_pp>{ws}*error{ws}*		yy_pp_state(pp_eol);	if(yy_top_state() != pp_ignore) return tERROR;
<pp_pp>{ws}*warning{ws}*	yy_pp_state(pp_eol);	if(yy_top_state() != pp_ignore) return tWARNING;
<pp_pp>{ws}*pragma{ws}*		yy_pp_state(pp_eol);	if(yy_top_state() != pp_ignore) return tPRAGMA;
<pp_pp>{ws}*ident{ws}*		yy_pp_state(pp_eol);	if(yy_top_state() != pp_ignore) return tPPIDENT;
<pp_pp>{ws}*undef{ws}*		if(yy_top_state() != pp_ignore) {yy_pp_state(pp_ifd); return tUNDEF;} else {yy_pp_state(pp_eol);}
<pp_pp>{ws}*ifdef{ws}*		yy_pp_state(pp_ifd);	return tIFDEF;
<pp_pp>{ws}*ifndef{ws}*		pp_incl_state.seen_junk--; yy_pp_state(pp_ifd);	return tIFNDEF;
<pp_pp>{ws}*if{ws}*		if(yy_top_state() != pp_ignore) {yy_pp_state(pp_if);} else {yy_pp_state(pp_ifignored);} return tIF;
<pp_pp>{ws}*elif{ws}*		yy_pp_state(pp_if);	return tELIF;
<pp_pp>{ws}*else{ws}*		yy_pp_state(pp_endif);  return tELSE;
<pp_pp>{ws}*endif{ws}*		yy_pp_state(pp_endif);  return tENDIF;
<pp_pp>{ws}*line{ws}*		if(yy_top_state() != pp_ignore) {yy_pp_state(pp_line); return tLINE;} else {yy_pp_state(pp_eol);}
<pp_pp>{ws}+			if(yy_top_state() != pp_ignore) {yy_pp_state(pp_line); return tGCCLINE;} else {yy_pp_state(pp_eol);}
<pp_pp>{ws}*[a-z]+		ppy_error("Invalid preprocessor token '%s'", ppy_text);
<pp_pp>\r?\n			newline(1); yy_pop_state(); return tNL;	/* This could be the null-token */
<pp_pp>\\\r?\n			newline(0);
<pp_pp>\\\r?			ppy_error("Preprocessor junk '%s'", ppy_text);
<pp_pp>.			return *ppy_text;

	/*
	 * Handle #include and #line
	 */
<pp_line>[0-9]+			return make_number(10, &ppy_lval, ppy_text, ppy_leng);
<pp_inc>\<			new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_iqs);
<pp_inc,pp_line>\"		new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_dqs);
<pp_inc,pp_line>{ws}+		;
<pp_inc,pp_line>\n		newline(1); yy_pop_state(); return tNL;
<pp_inc,pp_line>\\\r?\n		newline(0);
<pp_inc,pp_line>(\\\r?)|(.)	ppy_error(yy_current_state() == pp_inc ? "Trailing junk in #include" : "Trailing junk in #line");

	/*
	 * Ignore all input when a false clause is parsed
	 */
<pp_ignore>[^#/\\\n]+		;
<pp_ignore>\n			newline(1);
<pp_ignore>\\\r?\n		newline(0);
<pp_ignore>(\\\r?)|(.)		;

	/*
	 * Handle #if and #elif.
	 * These require conditionals to be evaluated, but we do not
	 * want to jam the scanner normally when we see these tokens.
	 * Note: tIDENT is handled below.
	 */

<pp_if>0[0-7]*{ul}?		return make_number(8, &ppy_lval, ppy_text, ppy_leng);
<pp_if>0[0-7]*[8-9]+{ul}?	ppy_error("Invalid octal digit");
<pp_if>[1-9][0-9]*{ul}?		return make_number(10, &ppy_lval, ppy_text, ppy_leng);
<pp_if>0[xX][0-9a-fA-F]+{ul}?	return make_number(16, &ppy_lval, ppy_text, ppy_leng);
<pp_if>0[xX]			ppy_error("Invalid hex number");
<pp_if>defined			yy_push_state(pp_defined); return tDEFINED;
<pp_if>"<<"			return tLSHIFT;
<pp_if>">>"			return tRSHIFT;
<pp_if>"&&"			return tLOGAND;
<pp_if>"||"			return tLOGOR;
<pp_if>"=="			return tEQ;
<pp_if>"!="			return tNE;
<pp_if>"<="			return tLTE;
<pp_if>">="			return tGTE;
<pp_if>\n			newline(1); yy_pop_state(); return tNL;
<pp_if>\\\r?\n			newline(0);
<pp_if>\\\r?			ppy_error("Junk in conditional expression");
<pp_if>{ws}+			;
<pp_if>\'			new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_sqs);
<pp_if>\"			ppy_error("String constants not allowed in conditionals");
<pp_if>.			return *ppy_text;

<pp_ifignored>[^\n]+		ppy_lval.sint = 0; return tSINT;
<pp_ifignored>\n		newline(1); yy_pop_state(); return tNL;

	/*
	 * Handle #ifdef, #ifndef and #undef
	 * to get only an untranslated/unexpanded identifier
	 */
<pp_ifd>{cident}	ppy_lval.cptr = xstrdup(ppy_text); return tIDENT;
<pp_ifd>{ws}+		;
<pp_ifd>\n		newline(1); yy_pop_state(); return tNL;
<pp_ifd>\\\r?\n		newline(0);
<pp_ifd>(\\\r?)|(.)	ppy_error("Identifier expected");

	/*
	 * Handle #else and #endif.
	 */
<pp_endif>{ws}+		;
<pp_endif>\n		newline(1); yy_pop_state(); return tNL;
<pp_endif>\\\r?\n	newline(0);
<pp_endif>.		ppy_error("Garbage after #else or #endif.");

	/*
	 * Handle the special 'defined' keyword.
	 * This is necessary to get the identifier prior to any
	 * substitutions.
	 */
<pp_defined>{cident}		yy_pop_state(); ppy_lval.cptr = xstrdup(ppy_text); return tIDENT;
<pp_defined>{ws}+		;
<pp_defined>(\()|(\))		return *ppy_text;
<pp_defined>\\\r?\n		newline(0);
<pp_defined>(\\.)|(\n)|(.)	ppy_error("Identifier expected");

	/*
	 * Handle #error, #warning, #pragma and #ident.
	 * Pass everything literally to the parser, which
	 * will act appropriately.
	 * Comments are stripped from the literal text.
	 */
<pp_eol>[^/\\\n]+		if(yy_top_state() != pp_ignore) { ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL; }
<pp_eol>\/[^/\\\n*]*		if(yy_top_state() != pp_ignore) { ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL; }
<pp_eol>(\\\r?)|(\/[^/*])	if(yy_top_state() != pp_ignore) { ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL; }
<pp_eol>\n			newline(1); yy_pop_state(); if(yy_current_state() != pp_ignore) { return tNL; }
<pp_eol>\\\r?\n			newline(0);

	/*
	 * Handle left side of #define
	 */
<pp_def>{cident}\(		ppy_lval.cptr = xstrdup(ppy_text); ppy_lval.cptr[ppy_leng-1] = '\0'; yy_pp_state(pp_macro);  return tMACRO;
<pp_def>{cident}		ppy_lval.cptr = xstrdup(ppy_text); yy_pp_state(pp_define); return tDEFINE;
<pp_def>{ws}+			;
<pp_def>\\\r?\n			newline(0);
<pp_def>(\\\r?)|(\n)|(.)	ppy_error("Identifier expected");

	/*
	 * Scan the substitution of a define
	 */
<pp_define>[^'"/\\\n]+		ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL;
<pp_define>(\\\r?)|(\/[^/*])	ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL;
<pp_define>\\\r?\n{ws}+		newline(0); ppy_lval.cptr = xstrdup(" "); return tLITERAL;
<pp_define>\\\r?\n		newline(0);
<pp_define>\n			newline(1); yy_pop_state(); return tNL;
<pp_define>\'			new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_sqs);
<pp_define>\"			new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_dqs);

	/*
	 * Scan the definition macro arguments
	 */
<pp_macro>\){ws}*		yy_pp_state(pp_mbody); return tMACROEND;
<pp_macro>{ws}+			;
<pp_macro>{cident}		ppy_lval.cptr = xstrdup(ppy_text); return tIDENT;
<pp_macro>,			return ',';
<pp_macro>"..."			return tELLIPSIS;
<pp_macro>(\\\r?)|(\n)|(.)|(\.\.?)	ppy_error("Argument identifier expected");
<pp_macro>\\\r?\n		newline(0);

	/*
	 * Scan the substitution of a macro
	 */
<pp_mbody>[^a-zA-Z0-9'"#/\\\n]+	ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL;
<pp_mbody>{cident}		ppy_lval.cptr = xstrdup(ppy_text); return tIDENT;
<pp_mbody>\#\#			return tCONCAT;
<pp_mbody>\#			return tSTRINGIZE;
<pp_mbody>[0-9][a-zA-Z0-9]*[^a-zA-Z0-9'"#/\\\n]*	ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL;
<pp_mbody>(\\\r?)|(\/[^/*'"#\\\n]*)	ppy_lval.cptr = xstrdup(ppy_text); return tLITERAL;
<pp_mbody>\\\r?\n{ws}+		newline(0); ppy_lval.cptr = xstrdup(" "); return tLITERAL;
<pp_mbody>\\\r?\n		newline(0);
<pp_mbody>\n			newline(1); yy_pop_state(); return tNL;
<pp_mbody>\'			new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_sqs);
<pp_mbody>\"			new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_dqs);

	/*
	 * Macro expansion text scanning.
	 * This state is active just after the identifier is scanned
	 * that triggers an expansion. We *must* delete the leading
	 * whitespace before we can start scanning for arguments.
	 *
	 * If we do not see a '(' as next trailing token, then we have
	 * a false alarm. We just continue with a nose-bleed...
	 */
<pp_macign>{ws}*/\(	yy_pp_state(pp_macscan);
<pp_macign>{ws}*\n	{
		if(yy_top_state() != pp_macscan)
			newline(0);
	}
<pp_macign>{ws}*\\\r?\n	newline(0);
<pp_macign>{ws}+|{ws}*\\\r?|.	{
		macexpstackentry_t *mac = pop_macro();
		yy_pop_state();
		put_buffer(mac->ppp->ident, strlen(mac->ppp->ident));
		put_buffer(ppy_text, ppy_leng);
		free_macro(mac);
	}

	/*
	 * Macro expansion argument text scanning.
	 * This state is active when a macro's arguments are being read for expansion.
	 */
<pp_macscan>\(	{
		if(++MACROPARENTHESES() > 1)
			add_text_to_macro(ppy_text, ppy_leng);
	}
<pp_macscan>\)	{
		if(--MACROPARENTHESES() == 0)
		{
			yy_pop_state();
			macro_add_arg(1);
		}
		else
			add_text_to_macro(ppy_text, ppy_leng);
	}
<pp_macscan>,		{
		if(MACROPARENTHESES() > 1)
			add_text_to_macro(ppy_text, ppy_leng);
		else
			macro_add_arg(0);
	}
<pp_macscan>\"		new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_dqs);
<pp_macscan>\'		new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_sqs);
<pp_macscan>"/*"	yy_push_state(pp_comment); add_text_to_macro(" ", 1);
<pp_macscan>\n		pp_status.line_number++; pp_status.char_number = 1; add_text_to_macro(ppy_text, ppy_leng);
<pp_macscan>([^/(),\\\n"']+)|(\/[^/*(),\\\n'"]*)|(\\\r?)|(.)	add_text_to_macro(ppy_text, ppy_leng);
<pp_macscan>\\\r?\n	newline(0);

	/*
	 * Comment handling (almost all start-conditions)
	 */
<INITIAL,pp_pp,pp_ignore,pp_eol,pp_inc,pp_if,pp_ifd,pp_endif,pp_defined,pp_def,pp_define,pp_macro,pp_mbody,RCINCL>"/*" yy_push_state(pp_comment);
<pp_comment>[^*\n]*|"*"+[^*/\n]*	;
<pp_comment>\n				newline(0);
<pp_comment>"*"+"/"			yy_pop_state();

	/*
	 * Remove C++ style comment (almost all start-conditions)
	 */
<INITIAL,pp_pp,pp_ignore,pp_eol,pp_inc,pp_if,pp_ifd,pp_endif,pp_defined,pp_def,pp_define,pp_macro,pp_mbody,pp_macscan,RCINCL>"//"[^\n]*	{
		if(ppy_text[ppy_leng-1] == '\\')
			ppy_warning("C++ style comment ends with an escaped newline (escape ignored)");
	}

	/*
	 * Single, double and <> quoted constants
	 */
<INITIAL,pp_macexp>\"		pp_incl_state.seen_junk++; new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_dqs);
<INITIAL,pp_macexp>\'		pp_incl_state.seen_junk++; new_string(); add_string(ppy_text, ppy_leng); yy_push_state(pp_sqs);
<pp_dqs>[^"\\\n]+		add_string(ppy_text, ppy_leng);
<pp_dqs>\"			{
		add_string(ppy_text, ppy_leng);
		yy_pop_state();
		switch(yy_current_state())
		{
		case pp_pp:
		case pp_define:
		case pp_mbody:
		case pp_inc:
		case RCINCL:
			if (yy_current_state()==RCINCL) yy_pop_state();
			ppy_lval.cptr = get_string();
			return tDQSTRING;
		case pp_line:
			ppy_lval.cptr = get_string();
			return tDQSTRING;
		default:
			put_string();
		}
	}
<pp_sqs>[^'\\\n]+		add_string(ppy_text, ppy_leng);
<pp_sqs>\'			{
		add_string(ppy_text, ppy_leng);
		yy_pop_state();
		switch(yy_current_state())
		{
		case pp_if:
		case pp_define:
		case pp_mbody:
			ppy_lval.cptr = get_string();
			return tSQSTRING;
		default:
			put_string();
		}
	}
<pp_iqs>[^\>\\\n]+		add_string(ppy_text, ppy_leng);
<pp_iqs>\>			{
		add_string(ppy_text, ppy_leng);
		yy_pop_state();
		ppy_lval.cptr = get_string();
		return tIQSTRING;
	}
<pp_dqs>\\\r?\n		{
		/*
		 * This is tricky; we need to remove the line-continuation
		 * from preprocessor strings, but OTOH retain them in all
		 * other strings. This is because the resource grammar is
		 * even more braindead than initially analysed and line-
		 * continuations in strings introduce, sigh, newlines in
		 * the output. There goes the concept of non-breaking, non-
		 * spacing whitespace.
		 */
		switch(yy_top_state())
		{
		case pp_pp:
		case pp_define:
		case pp_mbody:
		case pp_inc:
		case pp_line:
			newline(0);
			break;
		default:
			add_string(ppy_text, ppy_leng);
			newline(-1);
		}
	}
<pp_iqs,pp_dqs,pp_sqs>\\.	add_string(ppy_text, ppy_leng);
<pp_iqs,pp_dqs,pp_sqs>\n	{
		newline(1);
		add_string(ppy_text, ppy_leng);
		ppy_warning("Newline in string constant encountered (started line %d)", string_start());
	}

	/*
	 * Identifier scanning
	 */
<INITIAL,pp_if,pp_inc,pp_macexp>{cident}	{
		pp_entry_t *ppp;
		pp_incl_state.seen_junk++;
		if(!(ppp = pplookup(ppy_text)))
		{
			if(yy_current_state() == pp_inc)
				ppy_error("Expected include filename");

			else if(yy_current_state() == pp_if)
			{
				ppy_lval.cptr = xstrdup(ppy_text);
				return tIDENT;
			}
			else {
				if((yy_current_state()==INITIAL) && (strcasecmp(ppy_text,"RCINCLUDE")==0)){
					yy_push_state(RCINCL);
					return tRCINCLUDE;
				}
				else put_buffer(ppy_text, ppy_leng);
			}
		}
		else if(!ppp->expanding)
		{
			switch(ppp->type)
			{
			case def_special:
				expand_special(ppp);
				break;
			case def_define:
				expand_define(ppp);
				break;
			case def_macro:
				yy_push_state(pp_macign);
				push_macro(ppp);
				break;
			default:
				assert(0);
			}
		}
		else put_buffer(ppy_text, ppy_leng);
	}

	/*
	 * Everything else that needs to be passed and
	 * newline and continuation handling
	 */
<INITIAL,pp_macexp>[^a-zA-Z_#'"/\\\n \r\t\f\v]+|(\/|\\)[^a-zA-Z_/*'"\\\n \r\t\v\f]*	pp_incl_state.seen_junk++; put_buffer(ppy_text, ppy_leng);
<INITIAL,pp_macexp>{ws}+	put_buffer(ppy_text, ppy_leng);
<INITIAL>\n			newline(1);
<INITIAL>\\\r?\n		newline(0);
<INITIAL>\\\r?			pp_incl_state.seen_junk++; put_buffer(ppy_text, ppy_leng);

	/*
	 * Special catcher for macro argmument expansion to prevent
	 * newlines to propagate to the output or admin.
	 */
<pp_macexp>(\n)|(.)|(\\\r?(\n|.))	put_buffer(ppy_text, ppy_leng);

<RCINCL>[A-Za-z0-9_\.\\/]+ {
		ppy_lval.cptr = xstrdup(ppy_text);
		yy_pop_state();
		return tRCINCLUDEPATH;
	}

<RCINCL>{ws}+ ;

<RCINCL>\"		{
		new_string(); add_string(ppy_text,ppy_leng);yy_push_state(pp_dqs);
	}

	/*
	 * This is a 'catch-all' rule to discover errors in the scanner
	 * in an orderly manner.
	 */
<*>.		pp_incl_state.seen_junk++; ppy_warning("Unmatched text '%c' (0x%02x); please report\n", isprint(*ppy_text & 0xff) ? *ppy_text : ' ', *ppy_text);

<<EOF>>	{
		YY_BUFFER_STATE b = YY_CURRENT_BUFFER;
		bufferstackentry_t *bep = pop_buffer();

		if((!bep && pp_get_if_depth()) || (bep && pp_get_if_depth() != bep->if_depth))
			ppy_warning("Unmatched #if/#endif at end of file");

		if(!bep)
		{
			if(YY_START != INITIAL)
			{
				ppy_error("Unexpected end of file during preprocessing");
				BEGIN(INITIAL);
			}
			yyterminate();
		}
		else if(bep->should_pop == 2)
		{
			macexpstackentry_t *mac;
			mac = pop_macro();
			expand_macro(mac);
		}
		ppy__delete_buffer(b);
	}

%%
/*
 **************************************************************************
 * Support functions
 **************************************************************************
 */

#ifndef ppy_wrap
int ppy_wrap(void)
{
	return 1;
}
#endif


/*
 *-------------------------------------------------------------------------
 * Output newlines or set them as continuations
 *
 * Input: -1 - Don't count this one, but update local position (see pp_dqs)
 *	   0 - Line-continuation seen and cache output
 *	   1 - Newline seen and flush output
 *-------------------------------------------------------------------------
 */
static void newline(int dowrite)
{
	pp_status.line_number++;
	pp_status.char_number = 1;

	if(dowrite == -1)
		return;

	ncontinuations++;
	if(dowrite)
	{
		for(;ncontinuations; ncontinuations--)
			put_buffer("\n", 1);
	}
}


/*
 *-------------------------------------------------------------------------
 * Make a number out of an any-base and suffixed string
 *
 * Possible number extensions:
 * - ""		int
 * - "L"	long int
 * - "LL"	long long int
 * - "U"	unsigned int
 * - "UL"	unsigned long int
 * - "ULL"	unsigned long long int
 * - "LU"	unsigned long int
 * - "LLU"	unsigned long long int
 * - "LUL"	invalid
 *
 * FIXME:
 * The sizes of resulting 'int' and 'long' are compiler specific.
 *
 *-------------------------------------------------------------------------
 */
static int make_number(int radix, PPY_STYPE *val, const char *str, int len)
{
	int is_l  = 0;
	int is_ll = 0;
	int is_u  = 0;
	char ext[4];
	long l;

	ext[3] = '\0';
	ext[2] = toupper(str[len-1]);
	ext[1] = len > 1 ? toupper(str[len-2]) : ' ';
	ext[0] = len > 2 ? toupper(str[len-3]) : ' ';

	if(!strcmp(ext, "LUL"))
	{
		ppy_error("Invalid constant suffix");
		return 0;
	}
	else if(!strcmp(ext, "LLU") || !strcmp(ext, "ULL"))
	{
		is_ll++;
		is_u++;
	}
	else if(!strcmp(ext+1, "LU") || !strcmp(ext+1, "UL"))
	{
		is_l++;
		is_u++;
	}
	else if(!strcmp(ext+1, "LL"))
	{
		is_ll++;
	}
	else if(!strcmp(ext+2, "L"))
	{
		is_l++;
	}
	else if(!strcmp(ext+2, "U"))
	{
		is_u++;
	}

	if(is_u && is_ll)
	{
		errno = 0;
		val->ull = strtoull(str, NULL, radix);
		if (val->ull == ULLONG_MAX && errno == ERANGE)
		    ppy_error("integer constant %s is too large\n", str);
		return tULONGLONG;
	}
	else if(!is_u && is_ll)
	{
		errno = 0;
		val->sll = strtoll(str, NULL, radix);
		if ((val->sll == LLONG_MIN || val->sll == LLONG_MAX) && errno == ERANGE)
		    ppy_error("integer constant %s is too large\n", str);
		return tSLONGLONG;
	}
	else if(is_u && is_l)
	{
		errno = 0;
		val->ulong = strtoul(str, NULL, radix);
		if (val->ulong == ULONG_MAX && errno == ERANGE)
			ppy_error("integer constant %s is too large\n", str);
		return tULONG;
	}
	else if(!is_u && is_l)
	{
		errno = 0;
		val->slong = strtol(str, NULL, radix);
		if ((val->slong == LONG_MIN || val->slong == LONG_MAX) && errno == ERANGE)
			ppy_error("integer constant %s is too large\n", str);
		return tSLONG;
	}
	else if(is_u && !is_l)
	{
		unsigned long ul;
		errno = 0;
		ul = strtoul(str, NULL, radix);
		if ((ul == ULONG_MAX && errno == ERANGE) || (ul > UINT_MAX))
			ppy_error("integer constant %s is too large\n", str);
		val->uint = (unsigned int)ul;
		return tUINT;
	}

	/* Else it must be an int... */
	errno = 0;
	l = strtol(str, NULL, radix);
	if (((l == LONG_MIN || l == LONG_MAX) && errno == ERANGE) ||
		(l > INT_MAX) || (l < INT_MIN))
		ppy_error("integer constant %s is too large\n", str);
	val->sint = (int)l;
	return tSINT;
}


/*
 *-------------------------------------------------------------------------
 * Macro and define expansion support
 *
 * FIXME: Variable macro arguments.
 *-------------------------------------------------------------------------
 */
static void expand_special(pp_entry_t *ppp)
{
	static char *buf = NULL;

	assert(ppp->type == def_special);

	if(!strcmp(ppp->ident, "__LINE__"))
	{
		buf = xrealloc(buf, 32);
		snprintf(buf, 32, "%d", pp_status.line_number);
	}
	else if(!strcmp(ppp->ident, "__FILE__"))
	{
		buf = xrealloc(buf, strlen(pp_status.input) + 3);
		snprintf(buf, strlen(pp_status.input) + 3, "\"%s\"", pp_status.input);
	}

	if(pp_flex_debug)
		fprintf(stderr, "expand_special(%d): %s:%d: '%s' -> '%s'\n",
			macexpstackidx,
			pp_status.input,
			pp_status.line_number,
			ppp->ident,
			buf ? buf : "");

	if(buf && buf[0])
	{
		push_buffer(ppp, NULL, NULL, 0);
		yy_scan_string(buf);
	}
}

static void expand_define(pp_entry_t *ppp)
{
	assert(ppp->type == def_define);

	if(pp_flex_debug)
		fprintf(stderr, "expand_define(%d): %s:%d: '%s' -> '%s'\n",
			macexpstackidx,
			pp_status.input,
			pp_status.line_number,
			ppp->ident,
			ppp->subst.text);
	if(ppp->subst.text && ppp->subst.text[0])
	{
		push_buffer(ppp, NULL, NULL, 0);
		yy_scan_string(ppp->subst.text);
	}
}

static int curdef_idx = 0;
static int curdef_alloc = 0;
static char *curdef_text = NULL;

static void add_text(const char *str, int len)
{
	if(len == 0)
		return;
	if(curdef_idx >= curdef_alloc || curdef_alloc - curdef_idx < len)
	{
		curdef_alloc += (len + ALLOCBLOCKSIZE-1) & ~(ALLOCBLOCKSIZE-1);
		curdef_text = xrealloc(curdef_text, curdef_alloc * sizeof(curdef_text[0]));
	}
	memcpy(&curdef_text[curdef_idx], str, len);
	curdef_idx += len;
}

static mtext_t *add_expand_text(mtext_t *mtp, macexpstackentry_t *mep, int *nnl)
{
	char *cptr;
	char *exp;
	int tag;
	int n;

	if(mtp == NULL)
		return NULL;

	switch(mtp->type)
	{
	case exp_text:
		if(pp_flex_debug)
			fprintf(stderr, "add_expand_text: exp_text: '%s'\n", mtp->subst.text);
		add_text(mtp->subst.text, strlen(mtp->subst.text));
		break;

	case exp_stringize:
		if(pp_flex_debug)
			fprintf(stderr, "add_expand_text: exp_stringize(%d): '%s'\n",
				mtp->subst.argidx,
				mep->args[mtp->subst.argidx]);
		cptr = mep->args[mtp->subst.argidx];
		add_text("\"", 1);
		while(*cptr)
		{
			if(*cptr == '"' || *cptr == '\\')
				add_text("\\", 1);
			add_text(cptr, 1);
			cptr++;
		}
		add_text("\"", 1);
		break;

	case exp_concat:
		if(pp_flex_debug)
			fprintf(stderr, "add_expand_text: exp_concat\n");
		/* Remove trailing whitespace from current expansion text */
		while(curdef_idx)
		{
			if(isspace(curdef_text[curdef_idx-1] & 0xff))
				curdef_idx--;
			else
				break;
		}
		/* tag current position and recursively expand the next part */
		tag = curdef_idx;
		mtp = add_expand_text(mtp->next, mep, nnl);

		/* Now get rid of the leading space of the expansion */
		cptr = &curdef_text[tag];
		n = curdef_idx - tag;
		while(n)
		{
			if(isspace(*cptr & 0xff))
			{
				cptr++;
				n--;
			}
			else
				break;
		}
		if(cptr != &curdef_text[tag])
		{
			memmove(&curdef_text[tag], cptr, n);
			curdef_idx -= (curdef_idx - tag) - n;
		}
		break;

	case exp_subst:
		if((mtp->next && mtp->next->type == exp_concat) || (mtp->prev && mtp->prev->type == exp_concat))
			exp = mep->args[mtp->subst.argidx];
		else
			exp = mep->ppargs[mtp->subst.argidx];
		if(exp)
		{
			add_text(exp, strlen(exp));
			*nnl -= mep->nnls[mtp->subst.argidx];
			cptr = strchr(exp, '\n');
			while(cptr)
			{
				*cptr = ' ';
				cptr = strchr(cptr+1, '\n');
			}
			mep->nnls[mtp->subst.argidx] = 0;
		}
		if(pp_flex_debug)
			fprintf(stderr, "add_expand_text: exp_subst(%d): '%s'\n", mtp->subst.argidx, exp);
		break;
	}
	return mtp;
}

static void expand_macro(macexpstackentry_t *mep)
{
	mtext_t *mtp;
	int n, k;
	char *cptr;
	int nnl = 0;
	pp_entry_t *ppp = mep->ppp;
	int nargs = mep->nargs;

	assert(ppp->type == def_macro);
	assert(ppp->expanding == 0);

	if((!ppp->variadic && nargs != ppp->nargs) || (ppp->variadic && nargs < ppp->nargs))
	{
		ppy_error("Too %s macro arguments (%d)", nargs < ppp->nargs ? "few" : "many", nargs);
		return;
	}

	for(n = 0; n < nargs; n++)
		nnl += mep->nnls[n];

	if(pp_flex_debug)
		fprintf(stderr, "expand_macro(%d): %s:%d: '%s'(%d,%d) -> ...\n",
			macexpstackidx,
			pp_status.input,
			pp_status.line_number,
			ppp->ident,
			mep->nargs,
			nnl);

	curdef_idx = 0;

	for(mtp = ppp->subst.mtext; mtp; mtp = mtp->next)
	{
		if(!(mtp = add_expand_text(mtp, mep, &nnl)))
			break;
	}

	for(n = 0; n < nnl; n++)
		add_text("\n", 1);

	/* To make sure there is room and termination (see below) */
	add_text(" \0", 2);

	/* Strip trailing whitespace from expansion */
	for(k = curdef_idx, cptr = &curdef_text[curdef_idx-1]; k > 0; k--, cptr--)
	{
		if(!isspace(*cptr & 0xff))
			break;
	}

	/*
	 * We must add *one* whitespace to make sure that there
	 * is a token-separation after the expansion.
	 */
	*(++cptr) = ' ';
	*(++cptr) = '\0';
	k++;

	/* Strip leading whitespace from expansion */
	for(n = 0, cptr = curdef_text; n < k; n++, cptr++)
	{
		if(!isspace(*cptr & 0xff))
			break;
	}

	if(k - n > 0)
	{
		if(pp_flex_debug)
			fprintf(stderr, "expand_text: '%s'\n", curdef_text + n);
		push_buffer(ppp, NULL, NULL, 0);
		/*yy_scan_bytes(curdef_text + n, k - n);*/
		yy_scan_string(curdef_text + n);
	}
}

/*
 *-------------------------------------------------------------------------
 * String collection routines
 *-------------------------------------------------------------------------
 */
static void new_string(void)
{
	strbuf_idx = 0;
	str_startline = pp_status.line_number;
}

static void add_string(const char *str, int len)
{
	if(len == 0)
		return;
	if(strbuf_idx >= strbuf_alloc || strbuf_alloc - strbuf_idx < len)
	{
		strbuf_alloc += (len + ALLOCBLOCKSIZE-1) & ~(ALLOCBLOCKSIZE-1);
		strbuffer = xrealloc(strbuffer, strbuf_alloc * sizeof(strbuffer[0]));
	}
	memcpy(&strbuffer[strbuf_idx], str, len);
	strbuf_idx += len;
}

static char *get_string(void)
{
	char *str = xmalloc(strbuf_idx + 1);

	memcpy(str, strbuffer, strbuf_idx);
	str[strbuf_idx] = '\0';
	return str;
}

static void put_string(void)
{
	put_buffer(strbuffer, strbuf_idx);
}

static int string_start(void)
{
	return str_startline;
}


/*
 *-------------------------------------------------------------------------
 * Buffer management
 *-------------------------------------------------------------------------
 */
static void push_buffer(pp_entry_t *ppp, char *filename, char *incname, int pop)
{
	if(ppy_debug)
		printf("push_buffer(%d): %p %p %p %d\n", bufferstackidx, ppp, filename, incname, pop);
	if(bufferstackidx >= MAXBUFFERSTACK)
		error("Buffer stack overflow\n");

	memset(&bufferstack[bufferstackidx], 0, sizeof(bufferstack[0]));
	bufferstack[bufferstackidx].bufferstate	= YY_CURRENT_BUFFER;
	bufferstack[bufferstackidx].file        = pp_status.file;
	bufferstack[bufferstackidx].define	= ppp;
	bufferstack[bufferstackidx].line_number	= pp_status.line_number;
	bufferstack[bufferstackidx].char_number	= pp_status.char_number;
	bufferstack[bufferstackidx].if_depth	= pp_get_if_depth();
	bufferstack[bufferstackidx].should_pop	= pop;
	bufferstack[bufferstackidx].filename	= pp_status.input;
	bufferstack[bufferstackidx].ncontinuations	= ncontinuations;
	bufferstack[bufferstackidx].incl		= pp_incl_state;
	bufferstack[bufferstackidx].include_filename	= incname;

	if(ppp)
		ppp->expanding = 1;
	else if(filename)
	{
		/* These will track the ppy_error to the correct file and line */
		pp_status.line_number = 1;
		pp_status.char_number = 1;
		pp_status.input  = filename;
		ncontinuations = 0;
	}
	bufferstackidx++;
}

static bufferstackentry_t *pop_buffer(void)
{
	if(bufferstackidx == 0)
		return NULL;

	bufferstackidx--;

	if(bufferstack[bufferstackidx].define)
		bufferstack[bufferstackidx].define->expanding = 0;
	else
	{
		includelogicentry_t *iep = NULL;

		if(!bufferstack[bufferstackidx].should_pop)
		{
			fclose(pp_status.file);
			fprintf(ppy_out, "# %d \"%s\" 2\n", bufferstack[bufferstackidx].line_number, bufferstack[bufferstackidx].filename);

			/* We have EOF, check the include logic */
			if(pp_incl_state.state == 2 && !pp_incl_state.seen_junk && pp_incl_state.ppp)
			{
				pp_entry_t *ppp = pplookup(pp_incl_state.ppp);
				if(ppp)
				{
					iep = xmalloc(sizeof(includelogicentry_t));
					iep->ppp = ppp;
					ppp->iep = iep;
					iep->filename = bufferstack[bufferstackidx].include_filename;
                                        list_add_head( &pp_includelogiclist, &iep->entry );
					if(pp_status.debug)
						fprintf(stderr, "pop_buffer: %s:%d: includelogic added, include_ppp='%s', file='%s'\n",
                                                        bufferstack[bufferstackidx].filename, bufferstack[bufferstackidx].line_number, pp_incl_state.ppp, iep->filename);
				}
			}
			free(pp_incl_state.ppp);
			pp_incl_state	= bufferstack[bufferstackidx].incl;

		}
		if (bufferstack[bufferstackidx].include_filename)
		{
			free(pp_status.input);
			pp_status.input = bufferstack[bufferstackidx].filename;
		}
		pp_status.line_number = bufferstack[bufferstackidx].line_number;
		pp_status.char_number = bufferstack[bufferstackidx].char_number;
		ncontinuations = bufferstack[bufferstackidx].ncontinuations;
		if (!iep)
			free(bufferstack[bufferstackidx].include_filename);
	}

	if(ppy_debug)
		printf("pop_buffer(%d): %p %p (%d, %d, %d) %p %d\n",
			bufferstackidx,
			bufferstack[bufferstackidx].bufferstate,
			bufferstack[bufferstackidx].define,
			bufferstack[bufferstackidx].line_number,
			bufferstack[bufferstackidx].char_number,
			bufferstack[bufferstackidx].if_depth,
			bufferstack[bufferstackidx].filename,
			bufferstack[bufferstackidx].should_pop);

	pp_status.file = bufferstack[bufferstackidx].file;
	ppy__switch_to_buffer(bufferstack[bufferstackidx].bufferstate);

	if(bufferstack[bufferstackidx].should_pop)
	{
		assert( yy_current_state() == pp_macexp );
		macro_add_expansion();
		yy_pop_state();
	}

	return &bufferstack[bufferstackidx];
}


/*
 *-------------------------------------------------------------------------
 * Macro nestng support
 *-------------------------------------------------------------------------
 */
static void push_macro(pp_entry_t *ppp)
{
	if(macexpstackidx >= MAXMACEXPSTACK)
	{
		ppy_error("Too many nested macros");
		return;
	}

	macexpstack[macexpstackidx] = xmalloc(sizeof(macexpstack[0][0]));
        memset( macexpstack[macexpstackidx], 0, sizeof(macexpstack[0][0]));
	macexpstack[macexpstackidx]->ppp = ppp;
	macexpstackidx++;
}

static macexpstackentry_t *top_macro(void)
{
	return macexpstackidx > 0 ? macexpstack[macexpstackidx-1] : NULL;
}

static macexpstackentry_t *pop_macro(void)
{
	assert(macexpstackidx > 0);
	return macexpstack[--macexpstackidx];
}

static void free_macro(macexpstackentry_t *mep)
{
	int i;

	for(i = 0; i < mep->nargs; i++)
		free(mep->args[i]);
	free(mep->args);
	free(mep->nnls);
	free(mep->curarg);
	free(mep);
}

static void add_text_to_macro(const char *text, int len)
{
	macexpstackentry_t *mep = top_macro();

	assert(mep->ppp->expanding == 0);

	if(mep->curargalloc - mep->curargsize <= len+1)	/* +1 for '\0' */
	{
		mep->curargalloc += (ALLOCBLOCKSIZE > len+1) ? ALLOCBLOCKSIZE : len+1;
		mep->curarg = xrealloc(mep->curarg, mep->curargalloc * sizeof(mep->curarg[0]));
	}
	memcpy(mep->curarg + mep->curargsize, text, len);
	mep->curargsize += len;
	mep->curarg[mep->curargsize] = '\0';
}

static void macro_add_arg(int last)
{
	int nnl = 0;
	char *cptr;
	macexpstackentry_t *mep = top_macro();

	assert(mep->ppp->expanding == 0);

	mep->args = xrealloc(mep->args, (mep->nargs+1) * sizeof(mep->args[0]));
	mep->ppargs = xrealloc(mep->ppargs, (mep->nargs+1) * sizeof(mep->ppargs[0]));
	mep->nnls = xrealloc(mep->nnls, (mep->nargs+1) * sizeof(mep->nnls[0]));

	mep->args[mep->nargs] = xstrdup(mep->curarg ? mep->curarg : "");
	cptr = mep->args[mep->nargs]-1;
	while((cptr = strchr(cptr+1, '\n')))
	{
		nnl++;
	}
	mep->nnls[mep->nargs] = nnl;
	mep->nargs++;
	free(mep->curarg);
	mep->curargalloc = mep->curargsize = 0;
	mep->curarg = NULL;

	if(pp_flex_debug)
		fprintf(stderr, "macro_add_arg: %s:%d: %d -> '%s'\n",
			pp_status.input,
			pp_status.line_number,
			mep->nargs-1,
			mep->args[mep->nargs-1]);

	/* Each macro argument must be expanded to cope with stingize */
	if(last || mep->args[mep->nargs-1][0])
	{
		yy_push_state(pp_macexp);
		push_buffer(NULL, NULL, NULL, last ? 2 : 1);
		yy_scan_string(mep->args[mep->nargs-1]);
		/*mep->bufferstackidx = bufferstackidx;	 But not nested! */
	}
}

static void macro_add_expansion(void)
{
	macexpstackentry_t *mep = top_macro();

	assert(mep->ppp->expanding == 0);

	mep->ppargs[mep->nargs-1] = xstrdup(mep->curarg ? mep->curarg : "");
	free(mep->curarg);
	mep->curargalloc = mep->curargsize = 0;
	mep->curarg = NULL;

	if(pp_flex_debug)
		fprintf(stderr, "macro_add_expansion: %s:%d: %d -> '%s'\n",
			pp_status.input,
			pp_status.line_number,
			mep->nargs-1,
			mep->ppargs[mep->nargs-1] ? mep->ppargs[mep->nargs-1] : "");
}


/*
 *-------------------------------------------------------------------------
 * Output management
 *-------------------------------------------------------------------------
 */
static void put_buffer(const char *s, int len)
{
	if(top_macro())
		add_text_to_macro(s, len);
	else
		fwrite(s, 1, len, ppy_out);
}


/*
 *-------------------------------------------------------------------------
 * Include management
 *-------------------------------------------------------------------------
 */
void pp_do_include(char *fname, int type)
{
	char *newpath;
	int n;
	includelogicentry_t *iep;
	void *fp;

	if(!fname)
		return;

	LIST_FOR_EACH_ENTRY( iep, &pp_includelogiclist, includelogicentry_t, entry )
	{
		if(!strcmp(iep->filename, fname))
		{
			/*
			 * We are done. The file was included before.
			 * If the define was deleted, then this entry would have
			 * been deleted too.
			 */
			free(fname);
			return;
		}
	}

	n = strlen(fname);

	if(n <= 2)
	{
		ppy_error("Empty include filename");
		free(fname);
		return;
	}

	/* Undo the effect of the quotation */
	fname[n-1] = '\0';

	if((fp = pp_open_include(fname+1, type, pp_status.input, &newpath)) == NULL)
	{
		ppy_error("Unable to open include file %s", fname+1);
		free(fname);
		return;
	}

	fname[n-1] = *fname;	/* Redo the quotes */
	push_buffer(NULL, newpath, fname, 0);
	pp_incl_state.seen_junk = 0;
	pp_incl_state.state = 0;
	pp_incl_state.ppp = NULL;

	if(pp_status.debug)
		fprintf(stderr, "pp_do_include: %s:%d: include_state=%d, include_ifdepth=%d\n",
                        pp_status.input, pp_status.line_number, pp_incl_state.state, pp_incl_state.ifdepth);
	pp_status.file = fp;
	ppy__switch_to_buffer(ppy__create_buffer(NULL, YY_BUF_SIZE));

	fprintf(ppy_out, "# 1 \"%s\" 1%s\n", newpath, type ? "" : " 3");
}

/*
 *-------------------------------------------------------------------------
 * Push/pop preprocessor ignore state when processing conditionals
 * which are false.
 *-------------------------------------------------------------------------
 */
void pp_push_ignore_state(void)
{
	yy_push_state(pp_ignore);
}

void pp_pop_ignore_state(void)
{
	yy_pop_state();
}
