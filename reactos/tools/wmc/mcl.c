/*
 * Wine Message Compiler lexical scanner
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "config.h"

#include "utils.h"
#include "wmc.h"
#include "lang.h"

#include "y_tab.h"

/*
 * Keywords are case insenitive. All normal input is treated as
 * being in codepage iso-8859-1 for ascii input files (unicode
 * page 0) and as equivalent unicode if unicode input is selected.
 * All normal input, which is not part of a message text, is
 * enforced to be unicode page 0. Otherwise an error will be
 * generated. The normal file data should only be ASCII because
 * that is the basic definition of the grammar.
 *
 * Byteorder or unicode input is determined automatically by
 * reading the first 8 bytes and checking them against unicode
 * page 0 byteorder (hibyte must be 0).
 * -- FIXME --
 * Alternatively, the input is checked against a special byte
 * sequence to identify the file.
 * -- FIXME --
 *
 * 
 * Keywords:
 *	Codepages
 *	Facility
 *	FacilityNames
 *	LanguageNames
 *	MessageId
 *	MessageIdTypedef
 *	Severity
 *	SeverityNames
 *	SymbolicName
 *
 * Default added identifiers for classes:
 * SeverityNames:
 *	Success		= 0x0
 *	Informational	= 0x1
 *	Warning		= 0x2
 *	Error		= 0x3
 * FacilityNames:
 *	System		= 0x0FF
 *	Application	= 0xFFF
 *
 * The 'Codepages' keyword is a wmc extension.
 */

static WCHAR ustr_application[]		= { 'A', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', 0 };
static WCHAR ustr_codepages[]		= { 'C', 'o', 'd', 'e', 'p', 'a', 'g', 'e', 's', 0 };
static WCHAR ustr_english[]		= { 'E', 'n', 'g', 'l', 'i', 's', 'h', 0 };
static WCHAR ustr_error[]		= { 'E', 'r', 'r', 'o', 'r', 0 };
static WCHAR ustr_facility[]		= { 'F', 'a', 'c', 'i', 'l', 'i', 't', 'y', 0 };
static WCHAR ustr_facilitynames[]	= { 'F', 'a', 'c', 'i', 'l', 'i', 't', 'y', 'N', 'a', 'm', 'e', 's', 0 };
static WCHAR ustr_informational[]	= { 'I', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', 'a', 'l', 0 };
static WCHAR ustr_language[]		= { 'L', 'a', 'n', 'g', 'u', 'a', 'g', 'e', 0};
static WCHAR ustr_languagenames[]	= { 'L', 'a', 'n', 'g', 'u', 'a', 'g', 'e', 'N', 'a', 'm', 'e', 's', 0};
static WCHAR ustr_messageid[]		= { 'M', 'e', 's', 's', 'a', 'g', 'e', 'I', 'd', 0 };
static WCHAR ustr_messageidtypedef[]	= { 'M', 'e', 's', 's', 'a', 'g', 'e', 'I', 'd', 'T', 'y', 'p', 'e', 'd', 'e', 'f', 0 };
static WCHAR ustr_outputbase[]		= { 'O', 'u', 't', 'p', 'u', 't', 'B', 'a', 's', 'e', 0 };
static WCHAR ustr_severity[]		= { 'S', 'e', 'v', 'e', 'r', 'i', 't', 'y', 0 };
static WCHAR ustr_severitynames[]	= { 'S', 'e', 'v', 'e', 'r', 'i', 't', 'y', 'N', 'a', 'm', 'e', 's', 0 };
static WCHAR ustr_success[]		= { 'S', 'u', 'c', 'c', 'e', 's', 's', 0 };
static WCHAR ustr_symbolicname[]	= { 'S', 'y', 'm', 'b', 'o', 'l', 'i', 'c', 'N', 'a', 'm', 'e', 0 };
static WCHAR ustr_system[]		= { 'S', 'y', 's', 't', 'e', 'm', 0 };
static WCHAR ustr_warning[]		= { 'W', 'a', 'r', 'n', 'i', 'n', 'g', 0 };
static WCHAR ustr_msg00001[]		= { 'm', 's', 'g', '0', '0', '0', '0', '1', 0 };
/*
 * This table is to beat any form of "expression building" to check for
 * correct filename characters. It is also used for ident checks.
 * FIXME: use it more consistently.
 */

#define CH_SHORTNAME	0x01
#define CH_LONGNAME	0x02
#define CH_IDENT	0x04
#define CH_NUMBER	0x08
/*#define CH_WILDCARD	0x10*/
/*#define CH_DOT	0x20*/
#define CH_PUNCT	0x40
#define CH_INVALID	0x80

static const char char_table[256] = {
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, /* 0x00 - 0x07 */
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, /* 0x08 - 0x0F */
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, /* 0x10 - 0x17 */
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, /* 0x18 - 0x1F */
	0x80, 0x03, 0x80, 0x03, 0x03, 0x03, 0x03, 0x03, /* 0x20 - 0x27 " !"#$%&'" */
	0x43, 0x43, 0x10, 0x80, 0x03, 0x03, 0x22, 0x80, /* 0x28 - 0x2F "()*+,-./" */
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, /* 0x30 - 0x37 "01234567" */
	0x0b, 0x0b, 0xc0, 0x80, 0x80, 0x80, 0x80, 0x10, /* 0x38 - 0x3F "89:;<=>?" */
	0x03, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, /* 0x40 - 0x47 "@ABCDEFG" */
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, /* 0x48 - 0x4F "HIJKLMNO" */
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, /* 0x50 - 0x57 "PQRSTUVW" */
	0x07, 0x07, 0x07, 0x80, 0x80, 0x80, 0x80, 0x07, /* 0x58 - 0x5F "XYZ[\]^_" */
	0x03, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, /* 0x60 - 0x67 "`abcdefg" */
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, /* 0x68 - 0x6F "hijklmno" */
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, /* 0x70 - 0x77 "pqrstuvw" */
	0x07, 0x07, 0x07, 0x03, 0x80, 0x03, 0x03, 0x80, /* 0x78 - 0x7F "xyz{|}~ " */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0x80 - 0x87 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0x88 - 0x8F */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0x90 - 0x97 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0x98 - 0x9F */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xA0 - 0xA7 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xA8 - 0xAF */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xB0 - 0xB7 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xB8 - 0xBF */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xC0 - 0xC7 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xC8 - 0xCF */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xD0 - 0xD7 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xD8 - 0xDF */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xE0 - 0xE7 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xE8 - 0xEF */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xF0 - 0xF7 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x80, /* 0xF8 - 0xFF */
};

static int isisochar(int ch)
{
	return !(ch & (~0xff));
}

static int codepage;
//static const union cptable *codepage_def;

void set_codepage(int cp)
{
	codepage = cp;
#if 0
	codepage_def = find_codepage(codepage);
	if(!codepage_def)
		xyyerror("Codepage %d not found; cannot process", codepage);
#endif
}

/*
 * Input functions
 */
static int nungetstack = 0;
static int allocungetstack = 0;
static char *ungetstack = NULL;
static int ninputbuffer = 0;
static WCHAR *inputbuffer = NULL;
static char *xlatebuffer = NULL;

#define INPUTBUFFER_SIZE	2048	/* Must be larger than 4 and approx. large enough to hold a line */

/*
 * Fill the input buffer with *one* line of input.
 * The line is '\n' terminated so that scanning
 * messages with translation works as expected
 * (otherwise we cannot pre-translate because the
 * language is first known one line before the
 * actual message).
 */
static int fill_inputbuffer(void)
{
	int n;
	static char err_fatalread[] = "Fatal: reading input failed";
	static int endian = -1;

	if(!inputbuffer)
	{
		inputbuffer = xmalloc(INPUTBUFFER_SIZE);
		xlatebuffer = xmalloc(INPUTBUFFER_SIZE);
	}

try_again:
	if(!unicodein)
	{
		char *cptr;
		cptr = fgets(xlatebuffer, INPUTBUFFER_SIZE, yyin);
		if(!cptr && ferror(yyin))
			xyyerror(err_fatalread);
		else if(!cptr)
			return 0;
//		assert(codepage_def != NULL);
//		n = cp_mbstowcs(codepage_def, 0, xlatebuffer, strlen(xlatebuffer)+1, inputbuffer, INPUTBUFFER_SIZE);
		n = MultiByteToWideChar(codepage, 0, xlatebuffer, strlen(xlatebuffer)+1, inputbuffer, INPUTBUFFER_SIZE);		if(n < 0)
			internal_error(__FILE__, __LINE__, "Could not translate to unicode (%d)", n);
		if(n <= 1)
			goto try_again;	/* Should not hapen */
		n--;	/* Strip added conversion '\0' from input length */
		/*
		 * FIXME:
		 * Detect UTF-8 in the first time we read some bytes by
		 * checking the special sequence "FE..." or something like
		 * that. I need to check www.unicode.org for details.
		 */
	}
	else
	{
		if(endian == -1)
		{
			n = fread(inputbuffer, 1, 8, yyin);
			if(n != 8)
			{
				if(!n && ferror(yyin))
					xyyerror(err_fatalread);
				else
					xyyerror("Fatal: file to short to determine byteorder (should never happen)");
			}
			if(isisochar(inputbuffer[0]) &&
				isisochar(inputbuffer[1]) &&
				isisochar(inputbuffer[2]) &&
				isisochar(inputbuffer[3]))
			{
#ifdef WORDS_BIGENDIAN
				endian = WMC_BO_BIG;
#else
				endian = WMC_BO_LITTLE;
#endif
			}
			else if(isisochar(BYTESWAP_WORD(inputbuffer[0])) &&
				isisochar(BYTESWAP_WORD(inputbuffer[1])) &&
				isisochar(BYTESWAP_WORD(inputbuffer[2])) &&
				isisochar(BYTESWAP_WORD(inputbuffer[3])))
			{
#ifdef WORDS_BIGENDIAN
				endian = WMC_BO_LITTLE;
#else
				endian = WMC_BO_BIG;
#endif
			}
			else
				xyyerror("Fatal: cannot determine file's byteorder");
			/* FIXME:
			 * Determine the file-endian with the leader-bytes
			 * "FF FE..."; can't remember the exact sequence.
			 */
			n /= 2;
#ifdef WORDS_BIGENDIAN
			if(endian == WMC_BO_LITTLE)
#else
			if(endian == WMC_BO_BIG)
#endif
			{
				inputbuffer[0] = BYTESWAP_WORD(inputbuffer[0]);
				inputbuffer[1] = BYTESWAP_WORD(inputbuffer[1]);
				inputbuffer[2] = BYTESWAP_WORD(inputbuffer[2]);
				inputbuffer[3] = BYTESWAP_WORD(inputbuffer[3]);
			}

		}
		else
		{
			int i;
			n = 0;
			for(i = 0; i < INPUTBUFFER_SIZE; i++)
			{
				int t;
				t = fread(&inputbuffer[i], 2, 1, yyin);
				if(!t && ferror(yyin))
					xyyerror(err_fatalread);
				else if(!t && n)
					break;
				n++;
#ifdef WORDS_BIGENDIAN
				if(endian == WMC_BO_LITTLE)
#else
				if(endian == WMC_BO_BIG)
#endif
				{
					if((inputbuffer[i] = BYTESWAP_WORD(inputbuffer[i])) == '\n')
						break;
				}
				else
				{
					if(inputbuffer[i] == '\n')
						break;
				}
			}
		}

	}

	if(!n)
	{
		yywarning("Re-read line (input was or converted to zilch)");
		goto try_again;	/* Should not happen, but could be due to stdin reading and a signal */
	}

	ninputbuffer += n;
	return 1;
}

static int get_unichar(void)
{
	static WCHAR *b = NULL;
	char_number++;

	if(nungetstack)
		return ungetstack[--nungetstack];

	if(!ninputbuffer)
	{
		if(!fill_inputbuffer())
			return EOF;
		b = inputbuffer;
	}

	ninputbuffer--;
	return (int)(*b++ & 0xffff);
}

static void unget_unichar(int ch)
{
	if(ch == EOF)
		return;

	char_number--;

	if(nungetstack == allocungetstack)
	{
		allocungetstack += 32;
		ungetstack = xrealloc(ungetstack, allocungetstack * sizeof(*ungetstack));
	}

	ungetstack[nungetstack++] = (WCHAR)ch;
}


/*
 * Normal character stack.
 * Used for number scanning.
 */
static int ncharstack = 0;
static int alloccharstack = 0;
static char *charstack = NULL;

static void empty_char_stack(void)
{
	ncharstack = 0;
}

static void push_char(int ch)
{
	if(ncharstack == alloccharstack)
	{
		alloccharstack += 32;
		charstack = xrealloc(charstack, alloccharstack * sizeof(*charstack));
	}
	charstack[ncharstack++] = (char)ch;
}

static int tos_char_stack(void)
{
	if(!ncharstack)
		return 0;
	else
		return (int)(charstack[ncharstack-1] & 0xff);
}

static char *get_char_stack(void)
{
	return charstack;
}

/*
 * Unicode character stack.
 * Used for general scanner.
 */
static int nunicharstack = 0;
static int allocunicharstack = 0;
static WCHAR *unicharstack = NULL;

static void empty_unichar_stack(void)
{
	nunicharstack = 0;
}

static void push_unichar(int ch)
{
	if(nunicharstack == allocunicharstack)
	{
		allocunicharstack += 128;
		unicharstack = xrealloc(unicharstack, allocunicharstack * sizeof(*unicharstack));
	}
	unicharstack[nunicharstack++] = (WCHAR)ch;
}

#if 0
static int tos_unichar_stack(void)
{
	if(!nunicharstack)
		return 0;
	else
		return (int)(unicharstack[nunicharstack-1] & 0xffff);
}
#endif

static WCHAR *get_unichar_stack(void)
{
	return unicharstack;
}

/*
 * Number scanner
 *
 * state |      ch         | next state
 * ------+-----------------+--------------------------
 *   0   | [0]             | 1
 *   0   | [1-9]           | 4
 *   0   | .               | error (should never occur)
 *   1   | [xX]            | 2
 *   1   | [0-7]           | 3
 *   1   | [89a-wyzA-WYZ_] | error invalid digit
 *   1   | .               | return 0
 *   2   | [0-9a-fA-F]     | 2
 *   2   | [g-zG-Z_]       | error invalid hex digit
 *   2   | .               | return (hex-number) if TOS != [xX] else error
 *   3   | [0-7]           | 3
 *   3   | [89a-zA-Z_]     | error invalid octal digit
 *   3   | .               | return (octal-number)
 *   4   | [0-9]           | 4
 *   4   | [a-zA-Z_]       | error invalid decimal digit
 *   4   | .               | return (decimal-number)
 *
 * All non-identifier characters [^a-zA-Z_0-9] terminate the scan
 * and return the value. This is not entirely correct, but close
 * enough (should check punctuators as trailing context, but the
 * char_table is not adapted to that and it is questionable whether
 * it is worth the trouble).
 * All non-iso-8859-1 characters are an error.
 */
static int scan_number(int ch)
{
	int state = 0;
	int base = 10;
	empty_char_stack();

	while(1)
	{
		if(!isisochar(ch))
			xyyerror("Invalid digit");

		switch(state)
		{
		case 0:
			if(isdigit(ch))
			{
				push_char(ch);
				if(ch == '0')
					state = 1;
				else
					state = 4;
			}
			else
				internal_error(__FILE__, __LINE__, "Non-digit in first number-scanner state");
			break;
		case 1:
			if(ch == 'x' || ch == 'X')
			{
				push_char(ch);
				state = 2;
			}
			else if(ch >= '0' && ch <= '7')
			{
				push_char(ch);
				state = 3;
			}
			else if(isalpha(ch) || ch == '_')
				xyyerror("Invalid number digit");
			else
			{
				unget_unichar(ch);
				yylval.num = 0;
				return tNUMBER;
			}
			break;
		case 2:
			if(isxdigit(ch))
				push_char(ch);
			else if(isalpha(ch) || ch == '_' || !isxdigit(tos_char_stack()))
				xyyerror("Invalid hex digit");
			else
			{
				base = 16;
				goto finish;
			}
			break;
		case 3:
			if(ch >= '0' && ch <= '7')
				push_char(ch);
			else if(isalnum(ch) || ch == '_')
				xyyerror("Invalid octal digit");
			else
			{
				base = 8;
				goto finish;
			}
			break;
		case 4:
			if(isdigit(ch))
				push_char(ch);
			else if(isalnum(ch) || ch == '_')
				xyyerror("Invalid decimal digit");
			else
			{
				base = 10;
				goto finish;
			}
			break;
		default:
			internal_error(__FILE__, __LINE__, "Invalid state in number-scanner");
		}
		ch = get_unichar();
	}
finish:
	unget_unichar(ch);
	push_char(0);
	yylval.num = strtoul(get_char_stack(), NULL, base);
	return tNUMBER;
}

static void newline(void)
{
	line_number++;
	char_number = 1;
}

static int unisort(const void *p1, const void *p2)
{
	return unistricmp(((token_t *)p1)->name, ((token_t *)p2)->name);
}

static token_t *tokentable = NULL;
static int ntokentable = 0;

token_t *lookup_token(const WCHAR *s)
{
	token_t tok;

	tok.name = s;
	return (token_t *)bsearch(&tok, tokentable, ntokentable, sizeof(*tokentable), unisort);
}

void add_token(tok_e type, const WCHAR *name, int tok, int cp, const WCHAR *alias, int fix)
{
	ntokentable++;
	tokentable = xrealloc(tokentable, ntokentable * sizeof(*tokentable));
	tokentable[ntokentable-1].type = type;
	tokentable[ntokentable-1].name = name;
	tokentable[ntokentable-1].token = tok;
	tokentable[ntokentable-1].codepage = cp;
	tokentable[ntokentable-1].alias = alias;
	tokentable[ntokentable-1].fixed = fix;
	qsort(tokentable, ntokentable, sizeof(*tokentable), unisort);
}

void get_tokentable(token_t **tab, int *len)
{
	assert(tab != NULL);
	assert(len != NULL);
	*tab = tokentable;
	*len = ntokentable;
}

/*
 * The scanner
 *
 */
int yylex(void)
{
	static WCHAR ustr_dot1[] = { '.', '\n', 0 };
	static WCHAR ustr_dot2[] = { '.', '\r', '\n', 0 };
	static int isinit = 0;
	int ch;

	if(!isinit)
	{
		isinit++;
		set_codepage(WMC_DEFAULT_CODEPAGE);
		add_token(tok_keyword,	ustr_codepages,		tCODEPAGE,	0, NULL, 0);
		add_token(tok_keyword,	ustr_facility,		tFACILITY,	0, NULL, 1);
		add_token(tok_keyword,	ustr_facilitynames,	tFACNAMES,	0, NULL, 1);
		add_token(tok_keyword,	ustr_language,		tLANGUAGE,	0, NULL, 1);
		add_token(tok_keyword,	ustr_languagenames,	tLANNAMES,	0, NULL, 1);
		add_token(tok_keyword,	ustr_messageid,		tMSGID,		0, NULL, 1);
		add_token(tok_keyword,	ustr_messageidtypedef,	tTYPEDEF,	0, NULL, 1);
		add_token(tok_keyword,	ustr_outputbase,	tBASE,		0, NULL, 1);
		add_token(tok_keyword,	ustr_severity,		tSEVERITY,	0, NULL, 1);
		add_token(tok_keyword,	ustr_severitynames,	tSEVNAMES,	0, NULL, 1);
		add_token(tok_keyword,	ustr_symbolicname,	tSYMNAME,	0, NULL, 1);
		add_token(tok_severity,	ustr_error,		0x03,		0, NULL, 0);
		add_token(tok_severity,	ustr_warning,		0x02,		0, NULL, 0);
		add_token(tok_severity,	ustr_informational,	0x01,		0, NULL, 0);
		add_token(tok_severity,	ustr_success,		0x00,		0, NULL, 0);
		add_token(tok_facility,	ustr_application,	0xFFF,		0, NULL, 0);
		add_token(tok_facility,	ustr_system,		0x0FF,		0, NULL, 0);
		add_token(tok_language,	ustr_english,		0x409,		437, ustr_msg00001, 0);
	}

	empty_unichar_stack();

	while(1)
	{
		if(want_line)
		{
			while((ch = get_unichar()) != '\n')
			{
				if(ch == EOF)
					xyyerror("Unexpected EOF");
				push_unichar(ch);
			}
			newline();
			push_unichar(ch);
			push_unichar(0);
			if(!unistrcmp(ustr_dot1, get_unichar_stack()) || !unistrcmp(ustr_dot2, get_unichar_stack()))
			{
				want_line = 0;
				/* Reset the codepage to our default after each message */
				set_codepage(WMC_DEFAULT_CODEPAGE);
				return tMSGEND;
			}
			yylval.str = xunistrdup(get_unichar_stack());
			return tLINE;
		}

		ch = get_unichar();

		if(ch == EOF)
			return EOF;

		if(ch == '\n')
		{
			newline();
			if(want_nl)
			{
				want_nl = 0;
				return tNL;
			}
			continue;
		}

		if(isisochar(ch))
		{
			if(want_file)
			{
				int n = 0;
				while(n < 8 && isisochar(ch))
				{
					int t = char_table[ch];
					if((t & CH_PUNCT) || !(t & CH_SHORTNAME))
						break;
					
					push_unichar(ch);
					n++;
					ch = get_unichar();
				}
				unget_unichar(ch);
				push_unichar(0);
				want_file = 0;
				yylval.str = xunistrdup(get_unichar_stack());
				return tFILE;
			}

			if(char_table[ch] & CH_IDENT)
			{
				token_t *tok;
				while(isisochar(ch) && (char_table[ch] & (CH_IDENT|CH_NUMBER)))
				{
					push_unichar(ch);
					ch = get_unichar();
				}
				unget_unichar(ch);
				push_unichar(0);
				if(!(tok = lookup_token(get_unichar_stack())))
				{
					yylval.str = xunistrdup(get_unichar_stack());
					return tIDENT;
				}
				switch(tok->type)
				{
				case tok_keyword:
					return tok->token;

				case tok_language:
					codepage = tok->codepage;
					/* Fall through */
				case tok_severity:
				case tok_facility:
					yylval.tok = tok;
					return tTOKEN;

				default:
					internal_error(__FILE__, __LINE__, "Invalid token type encountered");
				}
			}

			if(isspace(ch))	/* Ignore space */
				continue;
	
			if(isdigit(ch))
				return scan_number(ch);
		}

		switch(ch)
		{
		case ':':
		case '=':
		case '+':
		case '(':
		case ')':
			return ch;
		case ';':
			while(ch != '\n' && ch != EOF)
			{
				push_unichar(ch);
				ch = get_unichar();
			}
			newline();
			push_unichar(ch);	/* Include the newline */
			push_unichar(0);
			yylval.str = xunistrdup(get_unichar_stack());
			return tCOMMENT;
		default:
			xyyerror("Invalid character '%c' (0x%04x)", isisochar(ch) && isprint(ch) ? ch : '.', ch);
		}
	}
}

