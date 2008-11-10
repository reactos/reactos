/*
 * Utility routines
 *
 * Copyright 1998 Bertho A. Stultiens
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "wine/unicode.h"
#include "wrc.h"
#include "utils.h"
#include "parser.h"

/* #define WANT_NEAR_INDICATION */

#ifdef WANT_NEAR_INDICATION
void make_print(char *str)
{
	while(*str)
	{
		if(!isprint(*str))
			*str = ' ';
		str++;
	}
}
#endif

static void generic_msg(const char *s, const char *t, const char *n, va_list ap)
{
	fprintf(stderr, "%s:%d:%d: %s: ", input_name ? input_name : "stdin", line_number, char_number, t);
	vfprintf(stderr, s, ap);
#ifdef WANT_NEAR_INDICATION
	{
		char *cpy;
		if(n)
		{
			cpy = xstrdup(n);
			make_print(cpy);
			fprintf(stderr, " near '%s'", cpy);
			free(cpy);
		}
	}
#endif
}


int parser_error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Error", parser_text, ap);
        fputc( '\n', stderr );
	va_end(ap);
	exit(1);
	return 1;
}

int parser_warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Warning", parser_text, ap);
	va_end(ap);
	return 0;
}

void internal_error(const char *file, int line, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Internal error (please report) %s %d: ", file, line);
	vfprintf(stderr, s, ap);
	va_end(ap);
	exit(3);
}

void error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
	exit(2);
}

void warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Warning: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
}

void chat(const char *s, ...)
{
	if(debuglevel & DEBUGLEVEL_CHAT)
	{
		va_list ap;
		va_start(ap, s);
		fprintf(stderr, "FYI: ");
		vfprintf(stderr, s, ap);
		va_end(ap);
	}
}

char *dup_basename(const char *name, const char *ext)
{
	int namelen;
	int extlen = strlen(ext);
	char *base;
	char *slash;

	if(!name)
		name = "wrc.tab";

	slash = strrchr(name, '/');
	if (slash)
		name = slash + 1;

	namelen = strlen(name);

	/* +4 for later extension and +1 for '\0' */
	base = (char *)xmalloc(namelen +4 +1);
	strcpy(base, name);
	if(!strcasecmp(name + namelen-extlen, ext))
	{
		base[namelen - extlen] = '\0';
	}
	return base;
}

void *xmalloc(size_t size)
{
    void *res;

    assert(size > 0);
    res = malloc(size);
    if(res == NULL)
    {
	error("Virtual memory exhausted.\n");
    }
    memset(res, 0x55, size);
    return res;
}


void *xrealloc(void *p, size_t size)
{
    void *res;

    assert(size > 0);
    res = realloc(p, size);
    if(res == NULL)
    {
	error("Virtual memory exhausted.\n");
    }
    return res;
}

char *xstrdup(const char *str)
{
	char *s;

	assert(str != NULL);
	s = (char *)xmalloc(strlen(str)+1);
	return strcpy(s, str);
}


/*
 *****************************************************************************
 * Function	: compare_name_id
 * Syntax	: int compare_name_id(const name_id_t *n1, const name_id_t *n2)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
int compare_name_id(const name_id_t *n1, const name_id_t *n2)
{
	if(n1->type == name_ord && n2->type == name_ord)
	{
		return n1->name.i_name - n2->name.i_name;
	}
	else if(n1->type == name_str && n2->type == name_str)
	{
		if(n1->name.s_name->type == str_char
		&& n2->name.s_name->type == str_char)
		{
			return strcasecmp(n1->name.s_name->str.cstr, n2->name.s_name->str.cstr);
		}
		else if(n1->name.s_name->type == str_unicode
		&& n2->name.s_name->type == str_unicode)
		{
			return strcmpiW(n1->name.s_name->str.wstr, n2->name.s_name->str.wstr);
		}
		else
		{
			internal_error(__FILE__, __LINE__, "Can't yet compare strings of mixed type\n");
		}
	}
	else if(n1->type == name_ord && n2->type == name_str)
		return 1;
	else if(n1->type == name_str && n2->type == name_ord)
		return -1;
	else
		internal_error(__FILE__, __LINE__, "Comparing name-ids with unknown types (%d, %d)\n",
				n1->type, n2->type);

	return 0; /* Keep the compiler happy */
}

string_t *convert_string(const string_t *str, enum str_e type, int codepage)
{
    const union cptable *cptable = codepage ? wine_cp_get_table( codepage ) : NULL;
    string_t *ret = xmalloc(sizeof(*ret));
    int res;

    if (!codepage && str->type != type)
        parser_error( "Current language is Unicode only, cannot convert string" );

    if((str->type == str_char) && (type == str_unicode))
    {
        ret->type = str_unicode;
        ret->size = cptable ? wine_cp_mbstowcs( cptable, 0, str->str.cstr, str->size, NULL, 0 )
                            : wine_utf8_mbstowcs( 0, str->str.cstr, str->size, NULL, 0 );
        ret->str.wstr = xmalloc( (ret->size+1) * sizeof(WCHAR) );
        if (cptable)
            res = wine_cp_mbstowcs( cptable, MB_ERR_INVALID_CHARS, str->str.cstr, str->size,
                                    ret->str.wstr, ret->size );
        else
            res = wine_utf8_mbstowcs( MB_ERR_INVALID_CHARS, str->str.cstr, str->size,
                                      ret->str.wstr, ret->size );
        if (res == -2)
            parser_error( "Invalid character in string '%.*s' for codepage %u",
                   str->size, str->str.cstr, codepage );
        ret->str.wstr[ret->size] = 0;
    }
    else if((str->type == str_unicode) && (type == str_char))
    {
        ret->type = str_char;
        ret->size = cptable ? wine_cp_wcstombs( cptable, 0, str->str.wstr, str->size, NULL, 0, NULL, NULL )
                            : wine_utf8_wcstombs( 0, str->str.wstr, str->size, NULL, 0 );
        ret->str.cstr = xmalloc( ret->size + 1 );
        if (cptable)
            wine_cp_wcstombs( cptable, 0, str->str.wstr, str->size, ret->str.cstr, ret->size, NULL, NULL );
        else
            wine_utf8_wcstombs( 0, str->str.wstr, str->size, ret->str.cstr, ret->size );
        ret->str.cstr[ret->size] = 0;
    }
    else if(str->type == str_unicode)
    {
        ret->type     = str_unicode;
        ret->size     = str->size;
        ret->str.wstr = xmalloc(sizeof(WCHAR)*(ret->size+1));
        memcpy( ret->str.wstr, str->str.wstr, ret->size * sizeof(WCHAR) );
        ret->str.wstr[ret->size] = 0;
    }
    else /* str->type == str_char */
    {
        ret->type     = str_char;
        ret->size     = str->size;
        ret->str.cstr = xmalloc( ret->size + 1 );
        memcpy( ret->str.cstr, str->str.cstr, ret->size );
        ret->str.cstr[ret->size] = 0;
    }
    return ret;
}


void free_string(string_t *str)
{
    if (str->type == str_unicode) free( str->str.wstr );
    else free( str->str.cstr );
    free( str );
}


int check_unicode_conversion( const string_t *str_a, const string_t *str_w, int codepage )
{
    int ok;
    string_t *teststr = convert_string( str_w, str_char, codepage );

    ok = (teststr->size == str_a->size && !memcmp( teststr->str.cstr, str_a->str.cstr, str_a->size ));

    if (!ok)
    {
        int i;

        fprintf( stderr, "Source: %s", str_a->str.cstr );
        for (i = 0; i < str_a->size; i++)
            fprintf( stderr, " %02x", (unsigned char)str_a->str.cstr[i] );
        fprintf( stderr, "\nUnicode: " );
        for (i = 0; i < str_w->size; i++)
            fprintf( stderr, " %04x", str_w->str.wstr[i] );
        fprintf( stderr, "\nBack: %s", teststr->str.cstr );
        for (i = 0; i < teststr->size; i++)
            fprintf( stderr, " %02x", (unsigned char)teststr->str.cstr[i] );
        fprintf( stderr, "\n" );
    }
    free_string( teststr );
    return ok;
}


struct lang2cp
{
    unsigned short lang;
    unsigned short sublang;
    unsigned int   cp;
} lang2cp_t;

/* language to codepage conversion table */
/* specific sublanguages need only be specified if their codepage */
/* differs from the default (SUBLANG_NEUTRAL) */
static const struct lang2cp lang2cps[] =
{
    { LANG_AFRIKAANS,      SUBLANG_NEUTRAL,              1252 },
    { LANG_ALBANIAN,       SUBLANG_NEUTRAL,              1250 },
    { LANG_ARABIC,         SUBLANG_NEUTRAL,              1256 },
    { LANG_ARMENIAN,       SUBLANG_NEUTRAL,              0    },
    { LANG_ASSAMESE,       SUBLANG_NEUTRAL,              0    },
    { LANG_AZERI,          SUBLANG_NEUTRAL,              1254 },
    { LANG_AZERI,          SUBLANG_AZERI_CYRILLIC,       1251 },
    { LANG_BASQUE,         SUBLANG_NEUTRAL,              1252 },
    { LANG_BELARUSIAN,     SUBLANG_NEUTRAL,              1251 },
    { LANG_BENGALI,        SUBLANG_NEUTRAL,              0    },
    { LANG_BRETON,         SUBLANG_NEUTRAL,              1252 },
    { LANG_BULGARIAN,      SUBLANG_NEUTRAL,              1251 },
    { LANG_CATALAN,        SUBLANG_NEUTRAL,              1252 },
    { LANG_CHINESE,        SUBLANG_NEUTRAL,              950  },
    { LANG_CHINESE,        SUBLANG_CHINESE_SIMPLIFIED,   936  },
    { LANG_CHINESE,        SUBLANG_CHINESE_SINGAPORE,    936  },
#ifdef LANG_CORNISH
    { LANG_CORNISH,        SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_CORNISH */
    { LANG_CROATIAN,       SUBLANG_NEUTRAL,              1250 },
    { LANG_CZECH,          SUBLANG_NEUTRAL,              1250 },
    { LANG_DANISH,         SUBLANG_NEUTRAL,              1252 },
    { LANG_DIVEHI,         SUBLANG_NEUTRAL,              0    },
    { LANG_DUTCH,          SUBLANG_NEUTRAL,              1252 },
    { LANG_ENGLISH,        SUBLANG_NEUTRAL,              1252 },
#ifdef LANG_ESPERANTO
    { LANG_ESPERANTO,      SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_ESPERANTO */
    { LANG_ESTONIAN,       SUBLANG_NEUTRAL,              1257 },
    { LANG_FAEROESE,       SUBLANG_NEUTRAL,              1252 },
    { LANG_FARSI,          SUBLANG_NEUTRAL,              1256 },
    { LANG_FINNISH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_FRENCH,         SUBLANG_NEUTRAL,              1252 },
#ifdef LANG_GAELIC
    { LANG_GAELIC,         SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_GAELIC */
    { LANG_GALICIAN,       SUBLANG_NEUTRAL,              1252 },
    { LANG_GEORGIAN,       SUBLANG_NEUTRAL,              0    },
    { LANG_GERMAN,         SUBLANG_NEUTRAL,              1252 },
    { LANG_GREEK,          SUBLANG_NEUTRAL,              1253 },
    { LANG_GUJARATI,       SUBLANG_NEUTRAL,              0    },
    { LANG_HEBREW,         SUBLANG_NEUTRAL,              1255 },
    { LANG_HINDI,          SUBLANG_NEUTRAL,              0    },
    { LANG_HUNGARIAN,      SUBLANG_NEUTRAL,              1250 },
    { LANG_ICELANDIC,      SUBLANG_NEUTRAL,              1252 },
    { LANG_INDONESIAN,     SUBLANG_NEUTRAL,              1252 },
    { LANG_IRISH,          SUBLANG_NEUTRAL,              1252 },
    { LANG_ITALIAN,        SUBLANG_NEUTRAL,              1252 },
    { LANG_JAPANESE,       SUBLANG_NEUTRAL,              932  },
    { LANG_KANNADA,        SUBLANG_NEUTRAL,              0    },
    { LANG_KAZAK,          SUBLANG_NEUTRAL,              1251 },
    { LANG_KONKANI,        SUBLANG_NEUTRAL,              0    },
    { LANG_KOREAN,         SUBLANG_NEUTRAL,              949  },
    { LANG_KYRGYZ,         SUBLANG_NEUTRAL,              1251 },
    { LANG_LATVIAN,        SUBLANG_NEUTRAL,              1257 },
    { LANG_LITHUANIAN,     SUBLANG_NEUTRAL,              1257 },
    { LANG_LOWER_SORBIAN,  SUBLANG_NEUTRAL,              1252 },
    { LANG_MACEDONIAN,     SUBLANG_NEUTRAL,              1251 },
    { LANG_MALAY,          SUBLANG_NEUTRAL,              1252 },
    { LANG_MALAYALAM,      SUBLANG_NEUTRAL,              0    },
    { LANG_MALTESE,        SUBLANG_NEUTRAL,              0    },
    { LANG_MARATHI,        SUBLANG_NEUTRAL,              0    },
    { LANG_MONGOLIAN,      SUBLANG_NEUTRAL,              1251 },
    { LANG_NEPALI,         SUBLANG_NEUTRAL,              0    },
    { LANG_NEUTRAL,        SUBLANG_NEUTRAL,              1252 },
    { LANG_NORWEGIAN,      SUBLANG_NEUTRAL,              1252 },
    { LANG_ORIYA,          SUBLANG_NEUTRAL,              0    },
    { LANG_POLISH,         SUBLANG_NEUTRAL,              1250 },
    { LANG_PORTUGUESE,     SUBLANG_NEUTRAL,              1252 },
    { LANG_PUNJABI,        SUBLANG_NEUTRAL,              0    },
    { LANG_ROMANIAN,       SUBLANG_NEUTRAL,              1250 },
    { LANG_ROMANSH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_RUSSIAN,        SUBLANG_NEUTRAL,              1251 },
    { LANG_SAMI,           SUBLANG_NEUTRAL,              1252 },
    { LANG_SANSKRIT,       SUBLANG_NEUTRAL,              0    },
    { LANG_SERBIAN,        SUBLANG_NEUTRAL,              1250 },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CYRILLIC,     1251 },
    { LANG_SLOVAK,         SUBLANG_NEUTRAL,              1250 },
    { LANG_SLOVENIAN,      SUBLANG_NEUTRAL,              1250 },
    { LANG_SPANISH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_SWAHILI,        SUBLANG_NEUTRAL,              1252 },
    { LANG_SWEDISH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_SYRIAC,         SUBLANG_NEUTRAL,              0    },
    { LANG_TAJIK,          SUBLANG_NEUTRAL,              1251 },
    { LANG_TAMIL,          SUBLANG_NEUTRAL,              0    },
    { LANG_TATAR,          SUBLANG_NEUTRAL,              1251 },
    { LANG_TELUGU,         SUBLANG_NEUTRAL,              0    },
    { LANG_THAI,           SUBLANG_NEUTRAL,              874  },
    { LANG_TSWANA,         SUBLANG_NEUTRAL,              1252 },
    { LANG_TURKISH,        SUBLANG_NEUTRAL,              1254 },
    { LANG_UKRAINIAN,      SUBLANG_NEUTRAL,              1251 },
    { LANG_UPPER_SORBIAN,  SUBLANG_NEUTRAL,              1252 },
    { LANG_URDU,           SUBLANG_NEUTRAL,              1256 },
    { LANG_UZBEK,          SUBLANG_NEUTRAL,              1254 },
    { LANG_UZBEK,          SUBLANG_UZBEK_CYRILLIC,       1251 },
    { LANG_VIETNAMESE,     SUBLANG_NEUTRAL,              1258 },
#ifdef LANG_WALON
    { LANG_WALON,          SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_WALON */
#ifdef LANG_WELSH
    { LANG_WELSH,          SUBLANG_NEUTRAL,              1252 },
#endif
    { LANG_XHOSA,          SUBLANG_NEUTRAL,              1252 },
    { LANG_ZULU,           SUBLANG_NEUTRAL,              1252 }
};

int get_language_codepage( unsigned short lang, unsigned short sublang )
{
    unsigned int i;
    int cp = -1, defcp = -1;

    for (i = 0; i < sizeof(lang2cps)/sizeof(lang2cps[0]); i++)
    {
        if (lang2cps[i].lang != lang) continue;
        if (lang2cps[i].sublang == sublang)
        {
            cp = lang2cps[i].cp;
            break;
        }
        if (lang2cps[i].sublang == SUBLANG_NEUTRAL) defcp = lang2cps[i].cp;
    }

    if (cp == -1) cp = defcp;
    assert( cp <= 0 || wine_cp_get_table(cp) );
    return cp;
}
