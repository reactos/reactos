/* -*-C-*-
 *
 * Copyright 1998-2000	Bertho A. Stultiens (BS)
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
 * History:
 * 21-May-2000 BS	- Fixed the ident requirement of resource names
 *			  which can be keywords.
 * 30-Apr-2000 BS	- Reintegration into the wine-tree
 * 11-Jan-2000 BS	- Very drastic cleanup because we don't have a
 *			  preprocessor in here anymore.
 * 02-Jan-2000 BS	- Removed the preprocessor code
 * 23-Dec-1999 BS	- Removed the copyright for Martin von Loewis.
 *			  There is really nothing left of his code in
 *			  this parser.
 * 20-Jun-1998 BS	- Changed the filename conversion. Filenames are
 *			  case-sensitive inder *nix, but not under dos.
 *			  default behaviour is to convert to lower case.
 *			- All backslashes are converted to forward and
 *			  both single and double slash is recognized as
 *			  MS/Borland does.
 *			- Fixed a bug in 'yywf' case that prevented
 *			  double quoted names to be scanned propperly.
 *
 * 19-May-1998 BS	- Started to build a preprocessor.
 *			- Changed keyword processing completely to
 *			  table-lookups.
 *
 * 20-Apr-1998 BS	- Added ';' comment stripping
 *
 * 17-Apr-1998 BS	- Made the win32 keywords optional when compiling in
 *			  16bit mode
 *
 * 15-Apr-1998 BS	- Changed string handling to include escapes
 *			- Added unicode string handling (no codepage
 *			  translation though).
 *			- 'Borrowed' the main idea of string scanning from
 *			  the flex manual pages.
 *			- Added conditional handling of scanning depending
 *			  on the state of the parser. This was mainly required
 *			  to distinguish a file to load or raw data that
 *			  follows. MS's definition of filenames is rather
 *			  complex... It can be unquoted or double quoted. If
 *			  double quoted, then the '\\' char is not automatically
 *			  escaped according to Borland's rc compiler, but it
 *			  accepts both "\\path\\file.rc" and "\path\file.rc".
 *			  This makes life very hard! I go for the escaped
 *			  version, as this seems to be the documented way...
 *			- Single quoted strings are now parsed and converted
 *			  here.
 *			- Added comment stripping. The implementation is
 *			  'borrowed' from the flex manpages.
 *			- Rebuild string processing so that it may contain
 *			  escaped '\0'.
 */

/* Exclusive string handling */
%x tkstr
/* Exclusive unicode string handling */
%x tklstr
/* Exclusive rcdata single quoted data handling */
%x tkrcd
/* Exclusive comment eating... */
%x comment
/* Set when stripping c-junk */
%x pp_cstrip
/* Set when scanning #line style directives */
%x pp_line
/* Set when scanning #pragma */
%x pp_pragma
%x pp_code_page

%option stack
%option noinput nounput noyy_top_state noyywrap
%option 8bit never-interactive
%option prefix="parser_"

/* Some shortcut definitions */
ws	[ \f\t\r]
cident	[a-zA-Z_][0-9a-zA-Z_]*

%{

/*#define LEX_DEBUG*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#ifndef HAVE_UNISTD_H
#define YY_NO_UNISTD_H
#endif

#include "wine/unicode.h"
#include "wrc.h"
#include "utils.h"
#include "parser.h"
#include "newstruc.h"

#include "parser.tab.h"

/* Always update the current character position within a line */
#define YY_USER_ACTION	char_number+=yyleng; wanted_id = want_id; want_id = 0;

static void addcchar(char c);
static void addwchar(WCHAR s);
static string_t *get_buffered_cstring(void);
static string_t *get_buffered_wstring(void);
static string_t *make_string(char *s);

static char *cbuffer;		/* Buffers for string collection */
static int cbufidx;
static int cbufalloc = 0;
static WCHAR *wbuffer;
static int wbufidx;
static int wbufalloc = 0;

static int current_codepage = -1;  /* use language default */

/*
 * This one is a bit tricky.
 * We set 'want_id' in the parser to get the first
 * identifier we get across in the scanner, but we
 * also want it to be reset at nearly any token we
 * see. Exceptions are:
 * - newlines
 * - comments
 * - whitespace
 *
 * The scanner will automatically reset 'want_id'
 * after *each* scanner reduction and puts is value
 * into the var below. In this way we can see the
 * state after the YY_RULE_SETUP (i.e. the user action;
 * see above) and don't have to worry too much when
 * it needs to be reset.
 */
static int wanted_id = 0;
static int save_wanted_id;	/* To save across comment reductions */

struct keyword {
	const char	*keyword;
	int		token;
	int		isextension;
	int		needcase;
	int		alwayskw;
};

static struct keyword keywords[] = {
	{ "ACCELERATORS",	tACCELERATORS,		0, 0, 0},
	{ "ALT",		tALT,			0, 0, 0},
	{ "ASCII",		tASCII,			0, 0, 0},
	{ "AUTO3STATE",		tAUTO3STATE,		1, 0, 0},
	{ "AUTOCHECKBOX",	tAUTOCHECKBOX,		1, 0, 0},
	{ "AUTORADIOBUTTON",	tAUTORADIOBUTTON,	1, 0, 0},
	{ "BEGIN",		tBEGIN,			0, 0, 0},
	{ "BITMAP",		tBITMAP,		0, 0, 0},
	{ "BLOCK",		tBLOCK,			0, 0, 0},
	{ "BUTTON",		tBUTTON,		1, 0, 0},
	{ "CAPTION",		tCAPTION,		0, 0, 0},
	{ "CHARACTERISTICS",	tCHARACTERISTICS,	1, 0, 0},
	{ "CHECKBOX",		tCHECKBOX,		0, 0, 0},
	{ "CHECKED",		tCHECKED,		0, 0, 0},
	{ "CLASS",		tCLASS,			0, 0, 0},
	{ "COMBOBOX",		tCOMBOBOX,		0, 0, 0},
	{ "CONTROL",		tCONTROL,		0, 0, 0},
	{ "CTEXT",		tCTEXT,			0, 0, 0},
	{ "CURSOR",		tCURSOR,		0, 0, 0},
	{ "DEFPUSHBUTTON",	tDEFPUSHBUTTON,		0, 0, 0},
	{ "DIALOG",		tDIALOG,		0, 0, 0},
	{ "DIALOGEX",		tDIALOGEX,		1, 0, 0},
	{ "DISCARDABLE",	tDISCARDABLE,		0, 0, 0},
	{ "DLGINIT",		tDLGINIT,		0, 0, 0},
	{ "EDITTEXT",		tEDITTEXT,		0, 0, 0},
	{ "END",		tEND,			0, 0, 0},
	{ "EXSTYLE",		tEXSTYLE,		0, 0, 0},
	{ "FILEFLAGS",		tFILEFLAGS,		0, 0, 0},
	{ "FILEFLAGSMASK",	tFILEFLAGSMASK,		0, 0, 0},
	{ "FILEOS",		tFILEOS,		0, 0, 0},
	{ "FILESUBTYPE",	tFILESUBTYPE,		0, 0, 0},
	{ "FILETYPE",		tFILETYPE,		0, 0, 0},
	{ "FILEVERSION",	tFILEVERSION,		0, 0, 0},
	{ "FIXED",		tFIXED,			0, 0, 0},
	{ "FONT",		tFONT,			0, 0, 0},
	{ "FONTDIR",		tFONTDIR,		0, 0, 0},	/* This is a Borland BRC extension */
	{ "GRAYED",		tGRAYED,		0, 0, 0},
	{ "GROUPBOX",		tGROUPBOX,		0, 0, 0},
	{ "HELP",		tHELP,			0, 0, 0},
	{ "HTML",		tHTML,			0, 0, 0},
	{ "ICON",		tICON,			0, 0, 0},
	{ "IMPURE",		tIMPURE,		0, 0, 0},
	{ "INACTIVE",		tINACTIVE,		0, 0, 0},
	{ "LANGUAGE",		tLANGUAGE,		1, 0, 1},
	{ "LISTBOX",		tLISTBOX,		0, 0, 0},
	{ "LOADONCALL",		tLOADONCALL,		0, 0, 0},
	{ "LTEXT",		tLTEXT,			0, 0, 0},
	{ "MENU",		tMENU,			0, 0, 0},
	{ "MENUBARBREAK",	tMENUBARBREAK,		0, 0, 0},
	{ "MENUBREAK",		tMENUBREAK,		0, 0, 0},
	{ "MENUEX",		tMENUEX,		1, 0, 0},
	{ "MENUITEM",		tMENUITEM,		0, 0, 0},
	{ "MESSAGETABLE",	tMESSAGETABLE,		1, 0, 0},
	{ "MOVEABLE",		tMOVEABLE,		0, 0, 0},
	{ "NOINVERT",		tNOINVERT,		0, 0, 0},
	{ "NOT",		tNOT,			0, 0, 0},
	{ "POPUP",		tPOPUP,			0, 0, 0},
	{ "PRELOAD",		tPRELOAD,		0, 0, 0},
	{ "PRODUCTVERSION",	tPRODUCTVERSION,	0, 0, 0},
	{ "PURE",		tPURE,			0, 0, 0},
	{ "PUSHBUTTON",		tPUSHBUTTON,		0, 0, 0},
	{ "RADIOBUTTON",	tRADIOBUTTON,		0, 0, 0},
	{ "RCDATA",		tRCDATA,		0, 0, 0},
	{ "RTEXT",		tRTEXT,			0, 0, 0},
	{ "SCROLLBAR",		tSCROLLBAR,		0, 0, 0},
	{ "SEPARATOR",		tSEPARATOR,		0, 0, 0},
	{ "SHIFT",		tSHIFT,			0, 0, 0},
	{ "STATE3",		tSTATE3,		1, 0, 0},
	{ "STRING",		tSTRING,		0, 0, 0},
	{ "STRINGTABLE",	tSTRINGTABLE,		0, 0, 1},
	{ "STYLE",		tSTYLE,			0, 0, 0},
	{ "TOOLBAR",		tTOOLBAR,		1, 0, 0},
	{ "VALUE",		tVALUE,			0, 0, 0},
	{ "VERSION",		tVERSION,		1, 0, 0},
	{ "VERSIONINFO",	tVERSIONINFO,		0, 0, 0},
	{ "VIRTKEY",		tVIRTKEY,		0, 0, 0}
};

#define NKEYWORDS	(sizeof(keywords)/sizeof(keywords[0]))
#define KWP(p)		((const struct keyword *)(p))
static int kw_cmp_func(const void *s1, const void *s2)
{
	int ret;
	ret = strcasecmp(KWP(s1)->keyword, KWP(s2)->keyword);
	if(!ret && (KWP(s1)->needcase || KWP(s2)->needcase))
		return strcmp(KWP(s1)->keyword, KWP(s2)->keyword);
	else
		return ret;
}

#define KW_BSEARCH
#define DO_SORT
static struct keyword *iskeyword(char *kw)
{
	struct keyword *kwp;
	struct keyword key;
	key.keyword = kw;
	key.needcase = 0;
#ifdef DO_SORT
	{
		/* Make sure that it is sorted for bsearsh */
		static int sorted = 0;
		if(!sorted)
		{
			qsort(keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
			sorted = 1;
		}
	}
#endif
#ifdef KW_BSEARCH
	kwp = bsearch(&key, keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
#else
	{
		int i;
		for(i = 0; i < NKEYWORDS; i++)
		{
			if(!kw_cmp_func(&key, &keywords[i]))
				break;
		}
		if(i < NKEYWORDS)
			kwp = &keywords[i];
		else
			kwp = NULL;
	}
#endif

	if(kwp == NULL || (kwp->isextension && !extensions))
		return NULL;
	else
		return kwp;
}

/* converts an integer in string form to an unsigned long and prints an error
 * on overflow */
static unsigned long xstrtoul(const char *nptr, char **endptr, int base)
{
    unsigned long l;

    errno = 0;
    l = strtoul(nptr, endptr, base);
    if (l == ULONG_MAX && errno == ERANGE)
        parser_error("integer constant %s is too large", nptr);
    return l;
}

%}

/*
 **************************************************************************
 * The flexer starts here
 **************************************************************************
 */
%%
	/*
	 * Catch the GCC-style line statements here and parse them.
	 * This has the advantage that you can #include at any
	 * stage in the resource file.
	 * The preprocessor generates line directives in the format:
	 * # <linenum> "filename" <codes>
	 *
	 * Codes can be a sequence of:
	 * - 1 start of new file
	 * - 2 returning to previous
	 * - 3 system header
	 * - 4 interpret as C-code
	 *
	 * 4 is not used and 1 mutually excludes 2
	 * Anyhow, we are not really interested in these at all
	 * because we only want to know the linenumber and
	 * filename.
	 */
<INITIAL,pp_cstrip>^{ws}*\#{ws}*pragma{ws}+	yy_push_state(pp_pragma);
<INITIAL,pp_cstrip>^{ws}*\#{ws}*	yy_push_state(pp_line);
<pp_line>[^\n]*	{
		int lineno, len;
		char *cptr;
		char *fname;
		yy_pop_state();
		lineno = (int)strtol(yytext, &cptr, 10);
		if(!lineno)
			parser_error("Malformed '#...' line-directive; invalid linenumber");
		fname = strchr(cptr, '"');
		if(!fname)
			parser_error("Malformed '#...' line-directive; missing filename");
		fname++;
		cptr = strchr(fname, '"');
		if(!cptr)
			parser_error("Malformed '#...' line-directive; missing terminating \"");
		*cptr = '\0';
		line_number = lineno - 1;	/* We didn't read the newline */
		input_name = xstrdup(fname);
                /* ignore contents of C include files */
                len = strlen(input_name);
                if (len > 1 && !strcasecmp( input_name + len - 2, ".h" ))
                    BEGIN(pp_cstrip);
                else
                    BEGIN(INITIAL);
	}

<pp_pragma>code_page[^\n]*	yyless(9); yy_pop_state(); yy_push_state(pp_code_page);
<pp_pragma>[^\n]*		yy_pop_state(); if (pedantic) parser_warning("Unrecognized #pragma directive '%s'\n",yytext);

<pp_code_page>\({ws}*default{ws}*\)[^\n]*	current_codepage = -1; yy_pop_state();
<pp_code_page>\({ws}*utf8{ws}*\)[^\n]*		current_codepage = CP_UTF8; yy_pop_state();
<pp_code_page>\({ws}*[0-9]+{ws}*\)[^\n]* {
        char *p = yytext;
        yy_pop_state();
        while (*p < '0' || *p > '9') p++;
        current_codepage = strtol( p, NULL, 10 );
        if (current_codepage != CP_UTF8 && !wine_cp_get_table( current_codepage ))
        {
            parser_error("Codepage %d not supported", current_codepage);
            current_codepage = 0;
        }
    }
<pp_code_page>[^\n]*	yy_pop_state(); parser_error("Malformed #pragma code_page directive");

	/*
	 * Strip everything until a ';' taking
	 * into account braces {} for structures,
	 * classes and enums.
	 */
<pp_cstrip>\n			line_number++; char_number = 1;
<pp_cstrip>.			; /* ignore */

\{			return tBEGIN;
\}			return tEND;

[0-9]+[lL]?		{ parser_lval.num = xstrtoul(yytext,  0, 10); return toupper(yytext[yyleng-1]) == 'L' ? tLNUMBER : tNUMBER; }
0[xX][0-9A-Fa-f]+[lL]?	{ parser_lval.num = xstrtoul(yytext,  0, 16); return toupper(yytext[yyleng-1]) == 'L' ? tLNUMBER : tNUMBER; }
0[oO][0-7]+[lL]?	{ parser_lval.num = xstrtoul(yytext+2, 0, 8); return toupper(yytext[yyleng-1]) == 'L' ? tLNUMBER : tNUMBER; }

	/*
	 * The next two rules scan identifiers and filenames.
	 * This is achieved by using the priority ruling
	 * of the scanner where a '.' is valid in a filename
	 * and *only* in a filename. In this case, the second
	 * rule will be reduced because it is longer.
	 */
[A-Za-z_0-9.]+		{
				struct keyword *tok = iskeyword(yytext);

				if(tok)
				{
					if(wanted_id && !tok->alwayskw)
					{
						parser_lval.str = make_string(yytext);
						return tIDENT;
					}
					else
						return tok->token;
				}
				else
				{
					parser_lval.str = make_string(yytext);
					return tIDENT;
				}
			}
[A-Za-z_0-9./\\]+		parser_lval.str = make_string(yytext); return tFILENAME;

	/*
	 * Wide string scanning
	 */
L\"			{
				yy_push_state(tklstr);
				wbufidx = 0;
				if(!win32)
					parser_warning("16bit resource contains unicode strings\n");
			}
<tklstr>\"{ws}+	|
<tklstr>\"		{
				yy_pop_state();
				parser_lval.str = get_buffered_wstring();
				return tSTRING;
			}
<tklstr>\\[0-7]{1,6}	{ /* octal escape sequence */
				unsigned int result;
				result = strtoul(yytext+1, 0, 8);
				if ( result > 0xffff )
					parser_error("Character constant out of range");
				addwchar((WCHAR)result);
			}
<tklstr>\\x[0-9a-fA-F]{4} {  /* hex escape sequence */
				unsigned int result;
				result = strtoul(yytext+2, 0, 16);
				addwchar((WCHAR)result);
			}
<tklstr>\\x[0-9a-fA-F]{1,3} {  parser_error("Invalid hex escape sequence '%s'", yytext); }

<tklstr>\\[0-9]+	parser_error("Bad escape sequence");
<tklstr>\\\n{ws}*	line_number++; char_number = 1; /* backslash at EOL continues string after leading whitespace on next line */
<tklstr>\\a		addwchar('\a');
<tklstr>\\b		addwchar('\b');
<tklstr>\\f		addwchar('\f');
<tklstr>\\n		addwchar('\n');
<tklstr>\\r		addwchar('\r');
<tklstr>\\t		addwchar('\t');
<tklstr>\\v		addwchar('\v');
<tklstr>\\.		addwchar(yytext[1]);
<tklstr>\\\r\n		addwchar(yytext[2]); line_number++; char_number = 1;
<tklstr>\"\"		addwchar('\"');		/* "bla""bla"  -> "bla\"bla" */
<tklstr>\\\"\"		addwchar('\"');		/* "bla\""bla" -> "bla\"bla" */
<tklstr>\"{ws}+\"	;			/* "bla" "bla" -> "blabla" */
<tklstr>[^\\\n\"]+	{
				char *yptr = yytext;
				while(*yptr)	/* FIXME: codepage translation */
					addwchar(*yptr++ & 0xff);
			}
<tklstr>\n		parser_error("Unterminated string");

	/*
	 * Normal string scanning
	 */
\"			yy_push_state(tkstr); cbufidx = 0;
<tkstr>\"{ws}+	|
<tkstr>\"		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return tSTRING;
			}
<tkstr>\\[0-7]{1,3}	{ /* octal escape sequence */
				int result;
				result = strtol(yytext+1, 0, 8);
				if ( result > 0xff )
					parser_error("Character constant out of range");
				addcchar((char)result);
			}
<tkstr>\\x[0-9a-fA-F]{2} {  /* hex escape sequence */
				int result;
				result = strtol(yytext+2, 0, 16);
				addcchar((char)result);
			}
<tkstr>\\x[0-9a-fA-F]	{  parser_error("Invalid hex escape sequence '%s'", yytext); }

<tkstr>\\[0-9]+		parser_error("Bad escape sequence");
<tkstr>\\\n{ws}*	line_number++; char_number = 1; /* backslash at EOL continues string after leading whitespace on next line */
<tkstr>\\a		addcchar('\a');
<tkstr>\\b		addcchar('\b');
<tkstr>\\f		addcchar('\f');
<tkstr>\\n		addcchar('\n');
<tkstr>\\r		addcchar('\r');
<tkstr>\\t		addcchar('\t');
<tkstr>\\v		addcchar('\v');
<tkstr>\\.		addcchar(yytext[1]);
<tkstr>\\\r\n		addcchar(yytext[2]); line_number++; char_number = 1;
<tkstr>[^\\\n\"]+	{
				char *yptr = yytext;
				while(*yptr)
					addcchar(*yptr++);
			}
<tkstr>\"\"		addcchar('\"');		/* "bla""bla"   -> "bla\"bla" */
<tkstr>\\\"\"		addcchar('\"');		/* "bla\""bla"  -> "bla\"bla" */
<tkstr>\"{ws}+\"	;			/* "bla" "bla"  -> "blabla" */
<tkstr>\n		parser_error("Unterminated string");

	/*
	 * Raw data scanning
	 */
\'			yy_push_state(tkrcd); cbufidx = 0;
<tkrcd>\'		{
				yy_pop_state();
				parser_lval.raw = new_raw_data();
				parser_lval.raw->size = cbufidx;
				parser_lval.raw->data = xmalloc(parser_lval.raw->size);
				memcpy(parser_lval.raw->data, cbuffer, parser_lval.raw->size);
				return tRAWDATA;
			}
<tkrcd>[0-9a-fA-F]{2}	{
				int result;
				result = strtol(yytext, 0, 16);
				addcchar((char)result);
			}
<tkrcd>{ws}+		;	/* Ignore space */
<tkrcd>\n		line_number++; char_number = 1;
<tkrcd>.		parser_error("Malformed data-line");

	/*
	 * Comment stripping
	 * Should never occur after preprocessing
	 */
<INITIAL,pp_cstrip>"/*"	{
				yy_push_state(comment);
				save_wanted_id = wanted_id;
				if(!no_preprocess)
					parser_warning("Found comments after preprocessing, please report\n");
			}
<comment>[^*\n]*	;
<comment>"*"+[^*/\n]*	;
<comment>\n		line_number++; char_number = 1;
<comment>"*"+"/"	yy_pop_state(); want_id = save_wanted_id;

;[^\n]*			want_id = wanted_id; /* not really comment, but left-over c-junk */
"//"[^\n]*		want_id = wanted_id; if(!no_preprocess) parser_warning("Found comments after preprocessing, please report\n");

\n			{
				want_id = wanted_id;
				line_number++;
				char_number = 1;
				if(want_nl)
				{
					want_nl = 0;
					return tNL;
				}
			}
{ws}+			want_id = wanted_id;	/* Eat whitespace */

<INITIAL>.		return yytext[0];

<*>.|\n			{
				/* Catch all rule to find any unmatched text */
				if(*yytext == '\n')
				{
					line_number++;
					char_number = 1;
				}
				parser_warning("Unmatched text '%c' (0x%02x) YY_START=%d\n",
					isprint(*yytext & 0xff) ? *yytext : '.', *yytext, YY_START);
			}

%%

/* These dup functions copy the enclosed '\0' from
 * the resource string.
 */
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

static void addwchar(WCHAR s)
{
	if(wbufidx >= wbufalloc)
	{
		wbufalloc += 1024;
		wbuffer = xrealloc(wbuffer, wbufalloc * sizeof(wbuffer[0]));
		if(wbufalloc > 65536)
			parser_warning("Reallocating wide string buffer larger than 64kB\n");
	}
	wbuffer[wbufidx++] = s;
}

static string_t *get_buffered_cstring(void)
{
    string_t *str = new_string();

    str->size = cbufidx;
    str->type = str_char;
    str->str.cstr = xmalloc(cbufidx+1);
    memcpy(str->str.cstr, cbuffer, cbufidx);
    str->str.cstr[cbufidx] = '\0';

    if (!current_codepage || current_codepage == -1 || !win32)  /* store as ANSI string */
    {
        if (!current_codepage) parser_error("Codepage set to Unicode only, cannot use ASCII string here");
        return str;
    }
    else  /* convert to Unicode before storing */
    {
        string_t *str_w = convert_string( str, str_unicode, current_codepage );
        if (!check_unicode_conversion( str, str_w, current_codepage ))
            parser_error("String %s does not convert identically to Unicode and back in codepage %d. "
                    "Try using a Unicode string instead", str->str.cstr, current_codepage );
        free_string( str );
        return str_w;
    }
}

static string_t *get_buffered_wstring(void)
{
	string_t *str = new_string();
	str->size = wbufidx;
	str->type = str_unicode;
	str->str.wstr = xmalloc((wbufidx+1)*sizeof(WCHAR));
	memcpy(str->str.wstr, wbuffer, wbufidx*sizeof(WCHAR));
	str->str.wstr[wbufidx] = 0;
	return str;
}

static string_t *make_string(char *s)
{
	string_t *str = new_string();
	str->size = strlen(s);
	str->type = str_char;
	str->str.cstr = xmalloc(str->size+1);
	memcpy(str->str.cstr, s, str->size+1);
	return str;
}
