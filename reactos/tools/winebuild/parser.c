/*
 * Spec file parser
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "build.h"

int current_line = 0;

static char ParseBuffer[512];
static char TokenBuffer[512];
static char *ParseNext = ParseBuffer;
static FILE *input_file;

static const char * const TypeNames[TYPE_NBTYPES] =
{
    "variable",     /* TYPE_VARIABLE */
    "pascal",       /* TYPE_PASCAL */
    "equate",       /* TYPE_ABS */
    "stub",         /* TYPE_STUB */
    "stdcall",      /* TYPE_STDCALL */
    "cdecl",        /* TYPE_CDECL */
    "varargs",      /* TYPE_VARARGS */
    "extern"        /* TYPE_EXTERN */
};

static const char * const FlagNames[] =
{
    "norelay",     /* FLAG_NORELAY */
    "noname",      /* FLAG_NONAME */
    "ret16",       /* FLAG_RET16 */
    "ret64",       /* FLAG_RET64 */
    "i386",        /* FLAG_I386 */
    "register",    /* FLAG_REGISTER */
    "interrupt",   /* FLAG_INTERRUPT */
    "private",     /* FLAG_PRIVATE */
    NULL
};

static int IsNumberString(const char *s)
{
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
}

inline static int is_token_separator( char ch )
{
    return (ch == '(' || ch == ')' || ch == '-');
}

/* get the next line from the input file, or return 0 if at eof */
static int get_next_line(void)
{
    ParseNext = ParseBuffer;
    current_line++;
    return (fgets(ParseBuffer, sizeof(ParseBuffer), input_file) != NULL);
}

static const char * GetToken( int allow_eol )
{
    char *p = ParseNext;
    char *token = TokenBuffer;

    for (;;)
    {
        /* remove initial white space */
        p = ParseNext;
        while (isspace(*p)) p++;

        if (*p == '\\' && p[1] == '\n')  /* line continuation */
        {
            if (!get_next_line())
            {
                if (!allow_eol) error( "Unexpected end of file\n" );
                return NULL;
            }
        }
        else break;
    }

    if ((*p == '\0') || (*p == '#'))
    {
        if (!allow_eol) error( "Declaration not terminated properly\n" );
        return NULL;
    }

    /*
     * Find end of token.
     */
    if (is_token_separator(*p))
    {
        /* a separator is always a complete token */
        *token++ = *p++;
    }
    else while (*p != '\0' && !is_token_separator(*p) && !isspace(*p))
    {
        if (*p == '\\') p++;
        if (*p) *token++ = *p++;
    }
    *token = '\0';
    ParseNext = p;
    return TokenBuffer;
}


/*******************************************************************
 *         ParseVariable
 *
 * Parse a variable definition.
 */
static int ParseVariable( ORDDEF *odp )
{
    char *endptr;
    int *value_array;
    int n_values;
    int value_array_size;
    const char *token;

    if (SpecType == SPEC_WIN32)
    {
        error( "'variable' not supported in Win32, use 'extern' instead\n" );
        return 0;
    }

    if (!(token = GetToken(0))) return 0;
    if (*token != '(')
    {
        error( "Expected '(' got '%s'\n", token );
        return 0;
    }

    n_values = 0;
    value_array_size = 25;
    value_array = xmalloc(sizeof(*value_array) * value_array_size);

    for (;;)
    {
        if (!(token = GetToken(0)))
        {
            free( value_array );
            return 0;
        }
	if (*token == ')')
	    break;

	value_array[n_values++] = strtol(token, &endptr, 0);
	if (n_values == value_array_size)
	{
	    value_array_size += 25;
	    value_array = xrealloc(value_array,
				   sizeof(*value_array) * value_array_size);
	}

	if (endptr == NULL || *endptr != '\0')
        {
            error( "Expected number value, got '%s'\n", token );
            free( value_array );
            return 0;
        }
    }

    odp->u.var.n_values = n_values;
    odp->u.var.values = xrealloc(value_array, sizeof(*value_array) * n_values);
    return 1;
}


/*******************************************************************
 *         ParseExportFunction
 *
 * Parse a function definition.
 */
static int ParseExportFunction( ORDDEF *odp )
{
    const char *token;
    unsigned int i;

    switch(SpecType)
    {
    case SPEC_WIN16:
        if (odp->type == TYPE_STDCALL)
        {
            error( "'stdcall' not supported for Win16\n" );
            return 0;
        }
        break;
    case SPEC_WIN32:
        if (odp->type == TYPE_PASCAL)
        {
            error( "'pascal' not supported for Win32\n" );
            return 0;
        }
        if (odp->flags & FLAG_INTERRUPT)
        {
            error( "'interrupt' not supported for Win32\n" );
            return 0;
        }
        break;
    default:
        break;
    }

    if (!(token = GetToken(0))) return 0;
    if (*token != '(')
    {
        error( "Expected '(' got '%s'\n", token );
        return 0;
    }

    for (i = 0; i < sizeof(odp->u.func.arg_types); i++)
    {
        if (!(token = GetToken(0))) return 0;
	if (*token == ')')
	    break;

        if (!strcmp(token, "word"))
            odp->u.func.arg_types[i] = 'w';
        else if (!strcmp(token, "s_word"))
            odp->u.func.arg_types[i] = 's';
        else if (!strcmp(token, "long") || !strcmp(token, "segptr"))
            odp->u.func.arg_types[i] = 'l';
        else if (!strcmp(token, "ptr"))
            odp->u.func.arg_types[i] = 'p';
	else if (!strcmp(token, "str"))
	    odp->u.func.arg_types[i] = 't';
	else if (!strcmp(token, "wstr"))
	    odp->u.func.arg_types[i] = 'W';
	else if (!strcmp(token, "segstr"))
	    odp->u.func.arg_types[i] = 'T';
        else if (!strcmp(token, "double"))
        {
            odp->u.func.arg_types[i++] = 'l';
            if (i < sizeof(odp->u.func.arg_types)) odp->u.func.arg_types[i] = 'l';
        }
        else
        {
            error( "Unknown argument type '%s'\n", token );
            return 0;
        }

        if (SpecType == SPEC_WIN32)
        {
            if (strcmp(token, "long") &&
                strcmp(token, "ptr") &&
                strcmp(token, "str") &&
                strcmp(token, "wstr") &&
                strcmp(token, "double"))
            {
                error( "Type '%s' not supported for Win32\n", token );
                return 0;
            }
        }
    }
    if ((*token != ')') || (i >= sizeof(odp->u.func.arg_types)))
    {
        error( "Too many arguments\n" );
        return 0;
    }

    odp->u.func.arg_types[i] = '\0';
    if (odp->type == TYPE_VARARGS)
        odp->flags |= FLAG_NORELAY;  /* no relay debug possible for varags entry point */

    if (!(token = GetToken(1)))
    {
        if (!strcmp( odp->name, "@" ))
        {
            error( "Missing handler name for anonymous function\n" );
            return 0;
        }
        odp->link_name = xstrdup( odp->name );
    }
    else
    {
        odp->link_name = xstrdup( token );
        if (strchr( odp->link_name, '.' ))
        {
            if (SpecType == SPEC_WIN16)
            {
                error( "Forwarded functions not supported for Win16\n" );
                return 0;
            }
            odp->flags |= FLAG_FORWARD;
        }
    }
    return 1;
}


/*******************************************************************
 *         ParseEquate
 *
 * Parse an 'equate' definition.
 */
static int ParseEquate( ORDDEF *odp )
{
    char *endptr;
    int value;
    const char *token;

    if (SpecType == SPEC_WIN32)
    {
        error( "'equate' not supported for Win32\n" );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    value = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
    {
        error( "Expected number value, got '%s'\n", token );
        return 0;
    }
    odp->u.abs.value = value;
    return 1;
}


/*******************************************************************
 *         ParseStub
 *
 * Parse a 'stub' definition.
 */
static int ParseStub( ORDDEF *odp )
{
    odp->u.func.arg_types[0] = '\0';
    odp->link_name = xstrdup("");
    return 1;
}


/*******************************************************************
 *         ParseExtern
 *
 * Parse an 'extern' definition.
 */
static int ParseExtern( ORDDEF *odp )
{
    const char *token;

    if (SpecType == SPEC_WIN16)
    {
        error( "'extern' not supported for Win16, use 'variable' instead\n" );
        return 0;
    }
    if (!(token = GetToken(1)))
    {
        if (!strcmp( odp->name, "@" ))
        {
            error( "Missing handler name for anonymous extern\n" );
            return 0;
        }
        odp->link_name = xstrdup( odp->name );
    }
    else
    {
        odp->link_name = xstrdup( token );
        if (strchr( odp->link_name, '.' )) odp->flags |= FLAG_FORWARD;
    }
    return 1;
}


/*******************************************************************
 *         ParseFlags
 *
 * Parse the optional flags for an entry point
 */
static const char *ParseFlags( ORDDEF *odp )
{
    unsigned int i;
    const char *token;

    do
    {
        if (!(token = GetToken(0))) break;
        for (i = 0; FlagNames[i]; i++)
            if (!strcmp( FlagNames[i], token )) break;
        if (!FlagNames[i])
        {
            error( "Unknown flag '%s'\n", token );
            return NULL;
        }
        odp->flags |= 1 << i;
        token = GetToken(0);
    } while (token && *token == '-');

    return token;
}

/*******************************************************************
 *         fix_export_name
 *
 * Fix an exported function name by removing a possible @xx suffix
 */
static void fix_export_name( char *name )
{
    char *p, *end = strrchr( name, '@' );
    if (!end || !end[1] || end == name) return;
    /* make sure all the rest is digits */
    for (p = end + 1; *p; p++) if (!isdigit(*p)) return;
    *end = 0;
}

/*******************************************************************
 *         ParseOrdinal
 *
 * Parse an ordinal definition.
 */
static int ParseOrdinal(int ordinal)
{
    const char *token;

    ORDDEF *odp = xmalloc( sizeof(*odp) );
    memset( odp, 0, sizeof(*odp) );
    EntryPoints[nb_entry_points++] = odp;

    if (!(token = GetToken(0))) goto error;

    for (odp->type = 0; odp->type < TYPE_NBTYPES; odp->type++)
        if (TypeNames[odp->type] && !strcmp( token, TypeNames[odp->type] ))
            break;

    if (odp->type >= TYPE_NBTYPES)
    {
        error( "Expected type after ordinal, found '%s' instead\n", token );
        goto error;
    }

    if (!(token = GetToken(0))) goto error;
    if (*token == '-' && !(token = ParseFlags( odp ))) goto error;

    odp->name = xstrdup( token );
    fix_export_name( odp->name );
    odp->lineno = current_line;
    odp->ordinal = ordinal;

    switch(odp->type)
    {
    case TYPE_VARIABLE:
        if (!ParseVariable( odp )) goto error;
        break;
    case TYPE_PASCAL:
    case TYPE_STDCALL:
    case TYPE_VARARGS:
    case TYPE_CDECL:
        if (!ParseExportFunction( odp )) goto error;
        break;
    case TYPE_ABS:
        if (!ParseEquate( odp )) goto error;
        break;
    case TYPE_STUB:
        if (!ParseStub( odp )) goto error;
        break;
    case TYPE_EXTERN:
        if (!ParseExtern( odp )) goto error;
        break;
    default:
        assert( 0 );
    }

#ifndef __i386__
    if (odp->flags & FLAG_I386)
    {
        /* ignore this entry point on non-Intel archs */
        EntryPoints[--nb_entry_points] = NULL;
        free( odp );
        return 1;
    }
#endif

    if (ordinal != -1)
    {
        if (!ordinal)
        {
            error( "Ordinal 0 is not valid\n" );
            goto error;
        }
        if (ordinal >= MAX_ORDINALS)
        {
            error( "Ordinal number %d too large\n", ordinal );
            goto error;
        }
        if (ordinal > Limit) Limit = ordinal;
        if (ordinal < Base) Base = ordinal;
        odp->ordinal = ordinal;
        if (Ordinals[ordinal])
        {
            error( "Duplicate ordinal %d\n", ordinal );
            goto error;
        }
        Ordinals[ordinal] = odp;
    }

    if (!strcmp( odp->name, "@" ) || odp->flags & FLAG_NONAME)
    {
        if (ordinal == -1)
        {
            error( "Nameless function needs an explicit ordinal number\n" );
            goto error;
        }
        if (SpecType != SPEC_WIN32)
        {
            error( "Nameless functions not supported for Win16\n" );
            goto error;
        }
        if (!strcmp( odp->name, "@" )) free( odp->name );
        else odp->export_name = odp->name;
        odp->name = NULL;
    }
    else Names[nb_names++] = odp;
    return 1;

error:
    EntryPoints[--nb_entry_points] = NULL;
    free( odp->name );
    free( odp );
    return 0;
}


static int name_compare( const void *name1, const void *name2 )
{
    ORDDEF *odp1 = *(ORDDEF **)name1;
    ORDDEF *odp2 = *(ORDDEF **)name2;
    return strcmp( odp1->name, odp2->name );
}

/*******************************************************************
 *         sort_names
 *
 * Sort the name array and catch duplicates.
 */
static void sort_names(void)
{
    int i;

    if (!nb_names) return;

    /* sort the list of names */
    qsort( Names, nb_names, sizeof(Names[0]), name_compare );

    /* check for duplicate names */
    for (i = 0; i < nb_names - 1; i++)
    {
        if (!strcmp( Names[i]->name, Names[i+1]->name ))
        {
            current_line = max( Names[i]->lineno, Names[i+1]->lineno );
            error( "'%s' redefined\n%s:%d: First defined here\n",
                   Names[i]->name, input_file_name,
                   min( Names[i]->lineno, Names[i+1]->lineno ) );
        }
    }
}


/*******************************************************************
 *         ParseTopLevel
 *
 * Parse a spec file.
 */
int ParseTopLevel( FILE *file )
{
    const char *token;

    input_file = file;
    current_line = 0;

    while (get_next_line())
    {
        if (!(token = GetToken(1))) continue;
        if (strcmp(token, "@") == 0)
        {
            if (SpecType != SPEC_WIN32)
            {
                error( "'@' ordinals not supported for Win16\n" );
                continue;
            }
            if (!ParseOrdinal( -1 )) continue;
        }
        else if (IsNumberString(token))
        {
            if (!ParseOrdinal( atoi(token) )) continue;
        }
        else
        {
            error( "Expected ordinal declaration, got '%s'\n", token );
            continue;
        }
        if ((token = GetToken(1))) error( "Syntax error near '%s'\n", token );
    }

    current_line = 0;  /* no longer parsing the input file */
    sort_names();
    return !nb_errors;
}


/*******************************************************************
 *         add_debug_channel
 */
static void add_debug_channel( const char *name )
{
    int i;

    for (i = 0; i < nb_debug_channels; i++)
        if (!strcmp( debug_channels[i], name )) return;

    debug_channels = xrealloc( debug_channels, (nb_debug_channels + 1) * sizeof(*debug_channels));
    debug_channels[nb_debug_channels++] = xstrdup(name);
}


/*******************************************************************
 *         parse_debug_channels
 *
 * Parse a source file and extract the debug channel definitions.
 */
int parse_debug_channels( const char *srcdir, const char *filename )
{
    FILE *file;
    int eol_seen = 1;

    file = open_input_file( srcdir, filename );
    while (fgets( ParseBuffer, sizeof(ParseBuffer), file ))
    {
        char *channel, *end, *p = ParseBuffer;

        p = ParseBuffer + strlen(ParseBuffer) - 1;
        if (!eol_seen)  /* continuation line */
        {
            eol_seen = (*p == '\n');
            continue;
        }
        if ((eol_seen = (*p == '\n'))) *p = 0;

        p = ParseBuffer;
        while (isspace(*p)) p++;
        if (!memcmp( p, "WINE_DECLARE_DEBUG_CHANNEL", 26 ) ||
            !memcmp( p, "WINE_DEFAULT_DEBUG_CHANNEL", 26 ))
        {
            p += 26;
            while (isspace(*p)) p++;
            if (*p != '(')
            {
                error( "invalid debug channel specification '%s'\n", ParseBuffer );
                goto next;
            }
            p++;
            while (isspace(*p)) p++;
            if (!isalpha(*p))
            {
                error( "invalid debug channel specification '%s'\n", ParseBuffer );
                goto next;
            }
            channel = p;
            while (isalnum(*p) || *p == '_') p++;
            end = p;
            while (isspace(*p)) p++;
            if (*p != ')')
            {
                error( "invalid debug channel specification '%s'\n", ParseBuffer );
                goto next;
            }
            *end = 0;
            add_debug_channel( channel );
        }
    next:
        current_line++;
    }
    close_input_file( file );
    return !nb_errors;
}
