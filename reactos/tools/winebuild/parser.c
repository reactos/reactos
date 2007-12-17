/*
 * Spec file parser
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997, 2004 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winglue.h"
#include "build.h"

int current_line = 0;

static char ParseBuffer[512];
static char TokenBuffer[512];
static char *ParseNext = ParseBuffer;
static FILE *input_file;

static const char *separator_chars;
static const char *comment_chars;

/* valid characters in ordinal names */
static const char valid_ordname_chars[] = "/$:-_@?abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

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
    "private",     /* FLAG_PRIVATE */
    "ordinal",     /* FLAG_ORDINAL */
    NULL
};

static int IsNumberString(const char *s)
{
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
}

static inline int is_token_separator( char ch )
{
    return strchr( separator_chars, ch ) != NULL;
}

static inline int is_token_comment( char ch )
{
    return strchr( comment_chars, ch ) != NULL;
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

    if ((*p == '\0') || is_token_comment(*p))
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


static ORDDEF *add_entry_point( DLLSPEC *spec )
{
    if (spec->nb_entry_points == spec->alloc_entry_points)
    {
        spec->alloc_entry_points += 128;
        spec->entry_points = xrealloc( spec->entry_points,
                                       spec->alloc_entry_points * sizeof(*spec->entry_points) );
    }
    return &spec->entry_points[spec->nb_entry_points++];
}

/*******************************************************************
 *         parse_spec_variable
 *
 * Parse a variable definition in a .spec file.
 */
static int parse_spec_variable( ORDDEF *odp, DLLSPEC *spec )
{
    char *endptr;
    int *value_array;
    int n_values;
    int value_array_size;
    const char *token;

    if (spec->type == SPEC_WIN32)
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
 *         parse_spec_export
 *
 * Parse an exported function definition in a .spec file.
 */
static int parse_spec_export( ORDDEF *odp, DLLSPEC *spec )
{
    const char *token;
    unsigned int i;

    switch(spec->type)
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
            if (get_ptr_size() == 4 && i < sizeof(odp->u.func.arg_types))
                odp->u.func.arg_types[i] = 'l';
        }
        else
        {
            error( "Unknown argument type '%s'\n", token );
            return 0;
        }

        if (spec->type == SPEC_WIN32)
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
            if (spec->type == SPEC_WIN16)
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
 *         parse_spec_equate
 *
 * Parse an 'equate' definition in a .spec file.
 */
static int parse_spec_equate( ORDDEF *odp, DLLSPEC *spec )
{
    char *endptr;
    int value;
    const char *token;

    if (spec->type == SPEC_WIN32)
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
    if (value < -0x8000 || value > 0xffff)
    {
        error( "Value %d for absolute symbol doesn't fit in 16 bits\n", value );
        value = 0;
    }
    odp->u.abs.value = value;
    return 1;
}


/*******************************************************************
 *         parse_spec_stub
 *
 * Parse a 'stub' definition in a .spec file
 */
static int parse_spec_stub( ORDDEF *odp, DLLSPEC *spec )
{
    odp->u.func.arg_types[0] = '\0';
    odp->link_name = xstrdup("");
    odp->flags |= FLAG_I386;  /* don't bother generating stubs for Winelib */
    return 1;
}


/*******************************************************************
 *         parse_spec_extern
 *
 * Parse an 'extern' definition in a .spec file.
 */
static int parse_spec_extern( ORDDEF *odp, DLLSPEC *spec )
{
    const char *token;

    if (spec->type == SPEC_WIN16)
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
 *         parse_spec_flags
 *
 * Parse the optional flags for an entry point in a .spec file.
 */
static const char *parse_spec_flags( ORDDEF *odp )
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
 *         parse_spec_ordinal
 *
 * Parse an ordinal definition in a .spec file.
 */
static int parse_spec_ordinal( int ordinal, DLLSPEC *spec )
{
    const char *token;
    size_t len;

    ORDDEF *odp = add_entry_point( spec );
    memset( odp, 0, sizeof(*odp) );

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
    if (*token == '-' && !(token = parse_spec_flags( odp ))) goto error;

    odp->name = xstrdup( token );
    odp->lineno = current_line;
    odp->ordinal = ordinal;

    len = strspn( odp->name, valid_ordname_chars );
    if (len < strlen( odp->name ))
    {
        error( "Character '%c' is not allowed in exported name '%s'\n", odp->name[len], odp->name );
        goto error;
    }

    switch(odp->type)
    {
    case TYPE_VARIABLE:
        if (!parse_spec_variable( odp, spec )) goto error;
        break;
    case TYPE_PASCAL:
    case TYPE_STDCALL:
    case TYPE_VARARGS:
    case TYPE_CDECL:
        if (!parse_spec_export( odp, spec )) goto error;
        break;
    case TYPE_ABS:
        if (!parse_spec_equate( odp, spec )) goto error;
        break;
    case TYPE_STUB:
        if (!parse_spec_stub( odp, spec )) goto error;
        break;
    case TYPE_EXTERN:
        if (!parse_spec_extern( odp, spec )) goto error;
        break;
    default:
        assert( 0 );
    }

    if ((target_cpu != CPU_x86) && (odp->flags & FLAG_I386))
    {
        /* ignore this entry point on non-Intel archs */
        spec->nb_entry_points--;
        return 1;
    }

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
        if (ordinal > spec->limit) spec->limit = ordinal;
        if (ordinal < spec->base) spec->base = ordinal;
        odp->ordinal = ordinal;
    }

    if (odp->type == TYPE_STDCALL && !(odp->flags & FLAG_PRIVATE))
    {
        if (!strcmp( odp->name, "DllRegisterServer" ) ||
            !strcmp( odp->name, "DllUnregisterServer" ) ||
            !strcmp( odp->name, "DllGetClassObject" ) ||
            !strcmp( odp->name, "DllGetVersion" ) ||
            !strcmp( odp->name, "DllInstall" ) ||
            !strcmp( odp->name, "DllCanUnloadNow" ))
        {
            warning( "Function %s should be marked private\n", odp->name );
            if (strcmp( odp->name, odp->link_name ))
                warning( "Function %s should not use a different internal name (%s)\n",
                         odp->name, odp->link_name );
        }
    }

    if (!strcmp( odp->name, "@" ) || odp->flags & (FLAG_NONAME | FLAG_ORDINAL))
    {
        if (ordinal == -1)
        {
            if (!strcmp( odp->name, "@" ))
                error( "Nameless function needs an explicit ordinal number\n" );
            else
                error( "Function imported by ordinal needs an explicit ordinal number\n" );
            goto error;
        }
        if (spec->type != SPEC_WIN32)
        {
            error( "Nameless functions not supported for Win16\n" );
            goto error;
        }
        if (!strcmp( odp->name, "@" ))
        {
            free( odp->name );
            odp->name = NULL;
        }
        else if (!(odp->flags & FLAG_ORDINAL))  /* -ordinal only affects the import library */
        {
            odp->export_name = odp->name;
            odp->name = NULL;
        }
    }
    return 1;

error:
    spec->nb_entry_points--;
    free( odp->name );
    return 0;
}


static int name_compare( const void *ptr1, const void *ptr2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)ptr1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)ptr2;
    const char *name1 = odp1->name ? odp1->name : odp1->export_name;
    const char *name2 = odp2->name ? odp2->name : odp2->export_name;
    return strcmp( name1, name2 );
}

/*******************************************************************
 *         assign_names
 *
 * Build the name array and catch duplicates.
 */
static void assign_names( DLLSPEC *spec )
{
    int i, j, nb_exp_names = 0;
    ORDDEF **all_names;

    spec->nb_names = 0;
    for (i = 0; i < spec->nb_entry_points; i++)
        if (spec->entry_points[i].name) spec->nb_names++;
        else if (spec->entry_points[i].export_name) nb_exp_names++;

    if (!spec->nb_names && !nb_exp_names) return;

    /* check for duplicates */

    all_names = xmalloc( (spec->nb_names + nb_exp_names) * sizeof(all_names[0]) );
    for (i = j = 0; i < spec->nb_entry_points; i++)
        if (spec->entry_points[i].name || spec->entry_points[i].export_name)
            all_names[j++] = &spec->entry_points[i];

    qsort( all_names, j, sizeof(all_names[0]), name_compare );

    for (i = 0; i < j - 1; i++)
    {
        const char *name1 = all_names[i]->name ? all_names[i]->name : all_names[i]->export_name;
        const char *name2 = all_names[i+1]->name ? all_names[i+1]->name : all_names[i+1]->export_name;
        if (!strcmp( name1, name2 ))
        {
            current_line = max( all_names[i]->lineno, all_names[i+1]->lineno );
            error( "'%s' redefined\n%s:%d: First defined here\n",
                   name1, input_file_name,
                   min( all_names[i]->lineno, all_names[i+1]->lineno ) );
        }
    }
    free( all_names );

    if (spec->nb_names)
    {
        spec->names = xmalloc( spec->nb_names * sizeof(spec->names[0]) );
        for (i = j = 0; i < spec->nb_entry_points; i++)
            if (spec->entry_points[i].name) spec->names[j++] = &spec->entry_points[i];

        /* sort the list of names */
        qsort( spec->names, spec->nb_names, sizeof(spec->names[0]), name_compare );
    }
}

/*******************************************************************
 *         assign_ordinals
 *
 * Build the ordinal array.
 */
static void assign_ordinals( DLLSPEC *spec )
{
    int i, count, ordinal;

    /* start assigning from base, or from 1 if no ordinal defined yet */

    spec->base = MAX_ORDINALS;
    spec->limit = 0;
    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ordinal = spec->entry_points[i].ordinal;
        if (ordinal == -1) continue;
        if (ordinal > spec->limit) spec->limit = ordinal;
        if (ordinal < spec->base) spec->base = ordinal;
    }
    if (spec->base == MAX_ORDINALS) spec->base = 1;
    if (spec->limit < spec->base) spec->limit = spec->base;

    count = max( spec->limit + 1, spec->base + spec->nb_entry_points );
    spec->ordinals = xmalloc( count * sizeof(spec->ordinals[0]) );
    memset( spec->ordinals, 0, count * sizeof(spec->ordinals[0]) );

    /* fill in all explicitly specified ordinals */
    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ordinal = spec->entry_points[i].ordinal;
        if (ordinal == -1) continue;
        if (spec->ordinals[ordinal])
        {
            current_line = max( spec->entry_points[i].lineno, spec->ordinals[ordinal]->lineno );
            error( "ordinal %d redefined\n%s:%d: First defined here\n",
                   ordinal, input_file_name,
                   min( spec->entry_points[i].lineno, spec->ordinals[ordinal]->lineno ) );
        }
        else spec->ordinals[ordinal] = &spec->entry_points[i];
    }

    /* now assign ordinals to the rest */
    for (i = 0, ordinal = spec->base; i < spec->nb_entry_points; i++)
    {
        if (spec->entry_points[i].ordinal != -1) continue;
        while (spec->ordinals[ordinal]) ordinal++;
        if (ordinal >= MAX_ORDINALS)
        {
            current_line = spec->entry_points[i].lineno;
            fatal_error( "Too many functions defined (max %d)\n", MAX_ORDINALS );
        }
        spec->entry_points[i].ordinal = ordinal;
        spec->ordinals[ordinal] = &spec->entry_points[i];
    }
    if (ordinal > spec->limit) spec->limit = ordinal;
}


/*******************************************************************
 *         parse_spec_file
 *
 * Parse a .spec file.
 */
int parse_spec_file( FILE *file, DLLSPEC *spec )
{
    const char *token;

    input_file = file;
    current_line = 0;

    comment_chars = "#;";
    separator_chars = "()-";

    while (get_next_line())
    {
        if (!(token = GetToken(1))) continue;
        if (strcmp(token, "@") == 0)
        {
            if (spec->type != SPEC_WIN32)
            {
                error( "'@' ordinals not supported for Win16\n" );
                continue;
            }
            if (!parse_spec_ordinal( -1, spec )) continue;
        }
        else if (IsNumberString(token))
        {
            if (!parse_spec_ordinal( atoi(token), spec )) continue;
        }
        else
        {
            error( "Expected ordinal declaration, got '%s'\n", token );
            continue;
        }
        if ((token = GetToken(1))) error( "Syntax error near '%s'\n", token );
    }

    current_line = 0;  /* no longer parsing the input file */
    assign_names( spec );
    assign_ordinals( spec );
    return !nb_errors;
}


/*******************************************************************
 *         parse_def_library
 *
 * Parse a LIBRARY declaration in a .def file.
 */
static int parse_def_library( DLLSPEC *spec )
{
    const char *token = GetToken(1);

    if (!token) return 1;
    if (strcmp( token, "BASE" ))
    {
        free( spec->file_name );
        spec->file_name = xstrdup( token );
        if (!(token = GetToken(1))) return 1;
    }
    if (strcmp( token, "BASE" ))
    {
        error( "Expected library name or BASE= declaration, got '%s'\n", token );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    if (strcmp( token, "=" ))
    {
        error( "Expected '=' after BASE, got '%s'\n", token );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    /* FIXME: do something with base address */

    return 1;
}


/*******************************************************************
 *         parse_def_stack_heap_size
 *
 * Parse a STACKSIZE or HEAPSIZE declaration in a .def file.
 */
static int parse_def_stack_heap_size( int is_stack, DLLSPEC *spec )
{
    const char *token = GetToken(0);
    char *end;
    unsigned long size;

    if (!token) return 0;
    size = strtoul( token, &end, 0 );
    if (*end)
    {
        error( "Invalid number '%s'\n", token );
        return 0;
    }
    if (is_stack) spec->stack_size = size / 1024;
    else spec->heap_size = size / 1024;
    if (!(token = GetToken(1))) return 1;
    if (strcmp( token, "," ))
    {
        error( "Expected ',' after size, got '%s'\n", token );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    /* FIXME: do something with reserve size */
    return 1;
}


/*******************************************************************
 *         parse_def_export
 *
 * Parse an export declaration in a .def file.
 */
static int parse_def_export( char *name, DLLSPEC *spec )
{
    int i, args;
    const char *token = GetToken(1);

    ORDDEF *odp = add_entry_point( spec );
    memset( odp, 0, sizeof(*odp) );

    odp->lineno = current_line;
    odp->ordinal = -1;
    odp->name = name;
    args = remove_stdcall_decoration( odp->name );
    if (args == -1) odp->type = TYPE_CDECL;
    else
    {
        odp->type = TYPE_STDCALL;
        args /= get_ptr_size();
        if (args >= sizeof(odp->u.func.arg_types))
        {
            error( "Too many arguments in stdcall function '%s'\n", odp->name );
            return 0;
        }
        for (i = 0; i < args; i++) odp->u.func.arg_types[i] = 'l';
    }

    /* check for optional internal name */

    if (token && !strcmp( token, "=" ))
    {
        if (!(token = GetToken(0))) goto error;
        odp->link_name = xstrdup( token );
        remove_stdcall_decoration( odp->link_name );
        token = GetToken(1);
    }
    else
    {
      odp->link_name = xstrdup( name );
    }

    /* check for optional ordinal */

    if (token && token[0] == '@')
    {
        int ordinal;

        if (!IsNumberString( token+1 ))
        {
            error( "Expected number after '@', got '%s'\n", token+1 );
            goto error;
        }
        ordinal = atoi( token+1 );
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
        odp->ordinal = ordinal;
        token = GetToken(1);
    }

    /* check for other optional keywords */

    if (token && !strcmp( token, "NONAME" ))
    {
        if (odp->ordinal == -1)
        {
            error( "NONAME requires an ordinal\n" );
            goto error;
        }
        odp->export_name = odp->name;
        odp->name = NULL;
        odp->flags |= FLAG_NONAME;
        token = GetToken(1);
    }
    if (token && !strcmp( token, "PRIVATE" ))
    {
        odp->flags |= FLAG_PRIVATE;
        token = GetToken(1);
    }
    if (token && !strcmp( token, "DATA" ))
    {
        odp->type = TYPE_EXTERN;
        token = GetToken(1);
    }
    if (token)
    {
        error( "Garbage text '%s' found at end of export declaration\n", token );
        goto error;
    }
    return 1;

error:
    spec->nb_entry_points--;
    free( odp->name );
    return 0;
}


/*******************************************************************
 *         parse_def_file
 *
 * Parse a .def file.
 */
int parse_def_file( FILE *file, DLLSPEC *spec )
{
    const char *token;
    int in_exports = 0;

    input_file = file;
    current_line = 0;

    comment_chars = ";";
    separator_chars = ",=";

    while (get_next_line())
    {
        if (!(token = GetToken(1))) continue;

        if (!strcmp( token, "LIBRARY" ) || !strcmp( token, "NAME" ))
        {
            if (!parse_def_library( spec )) continue;
            goto end_of_line;
        }
        else if (!strcmp( token, "STACKSIZE" ))
        {
            if (!parse_def_stack_heap_size( 1, spec )) continue;
            goto end_of_line;
        }
        else if (!strcmp( token, "HEAPSIZE" ))
        {
            if (!parse_def_stack_heap_size( 0, spec )) continue;
            goto end_of_line;
        }
        else if (!strcmp( token, "EXPORTS" ))
        {
            in_exports = 1;
            if (!(token = GetToken(1))) continue;
        }
        else if (!strcmp( token, "IMPORTS" ))
        {
            in_exports = 0;
            if (!(token = GetToken(1))) continue;
        }
        else if (!strcmp( token, "SECTIONS" ))
        {
            in_exports = 0;
            if (!(token = GetToken(1))) continue;
        }

        if (!in_exports) continue;  /* ignore this line */
        if (!parse_def_export( xstrdup(token), spec )) continue;

    end_of_line:
        if ((token = GetToken(1))) error( "Syntax error near '%s'\n", token );
    }

    current_line = 0;  /* no longer parsing the input file */
    assign_names( spec );
    assign_ordinals( spec );
    return !nb_errors;
}
