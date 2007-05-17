/*
 * Exported functions of the Wine preprocessor
 *
 * Copyrignt 1998 Bertho A. Stultiens
 * Copyright 2002 Alexandre Julliard
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

#include <time.h>
#include <stdlib.h>

#include "wpp_private.h"
#include "wine/wpp.h"

int ppy_debug, pp_flex_debug;

struct define
{
    struct define *next;
    char          *name;
    char          *value;
};

static struct define *cmdline_defines;

static void add_cmdline_defines(void)
{
    struct define *def;

    for (def = cmdline_defines; def; def = def->next)
    {
        if (def->value) pp_add_define( pp_xstrdup(def->name), pp_xstrdup(def->value) );
    }
}

static void add_special_defines(void)
{
    time_t now = time(NULL);
    pp_entry_t *ppp;
    char buf[32];

    strftime(buf, sizeof(buf), "\"%b %d %Y\"", localtime(&now));
    pp_add_define( pp_xstrdup("__DATE__"), pp_xstrdup(buf) );

    strftime(buf, sizeof(buf), "\"%H:%M:%S\"", localtime(&now));
    pp_add_define( pp_xstrdup("__TIME__"), pp_xstrdup(buf) );

    ppp = pp_add_define( pp_xstrdup("__FILE__"), pp_xstrdup("") );
    ppp->type = def_special;

    ppp = pp_add_define( pp_xstrdup("__LINE__"), pp_xstrdup("") );
    ppp->type = def_special;
}


/* add a define to the preprocessor list */
void wpp_add_define( const char *name, const char *value )
{
    struct define *def;

    if (!value) value = "";

    for (def = cmdline_defines; def; def = def->next)
    {
        if (!strcmp( def->name, name ))
        {
            free( def->value );
            def->value = pp_xstrdup(value);
            return;
        }
    }

    def = pp_xmalloc( sizeof(*def) );
    def->next  = cmdline_defines;
    def->name  = pp_xstrdup(name);
    def->value = pp_xstrdup(value);
    cmdline_defines = def;
}


/* undefine a previously added definition */
void wpp_del_define( const char *name )
{
    struct define *def;

    for (def = cmdline_defines; def; def = def->next)
    {
        if (!strcmp( def->name, name ))
        {
            free( def->value );
            def->value = NULL;
            return;
        }
    }
}


/* add a command-line define of the form NAME=VALUE */
void wpp_add_cmdline_define( const char *value )
{
    char *str = pp_xstrdup(value);
    char *p = strchr( str, '=' );
    if (p) *p++ = 0;
    wpp_add_define( str, p );
    free( str );
}


/* set the various debug flags */
void wpp_set_debug( int lex_debug, int parser_debug, int msg_debug )
{
    pp_flex_debug   = lex_debug;
    ppy_debug       = parser_debug;
    pp_status.debug = msg_debug;
}


/* set the pedantic mode */
void wpp_set_pedantic( int on )
{
    pp_status.pedantic = on;
}


/* the main preprocessor parsing loop */
int wpp_parse( const char *input, FILE *output )
{
    int ret;

    pp_status.input = NULL;

    pp_push_define_state();
    add_cmdline_defines();
    add_special_defines();

    if (!input) ppy_in = stdin;
    else if (!(ppy_in = fopen(input, "rt")))
    {
        fprintf(stderr,"Could not open %s\n", input);
        exit(2);
    }

    pp_status.input = input;

    ppy_out = output;
    fprintf(ppy_out, "# 1 \"%s\" 1\n", input ? input : "");

    ret = ppy_parse();

    if (input) fclose(ppy_in);
    pp_pop_define_state();
    return ret;
}


/* parse into a temporary file */
int wpp_parse_temp( const char *input, const char *output_base, char **output_name )
{
    FILE *output;
    int ret, fd;
    char *temp_name;

    if (!output_base || !output_base[0]) output_base = "wpptmp";

    temp_name = pp_xmalloc( strlen(output_base) + 8 );
    strcpy( temp_name, output_base );
    strcat( temp_name, ".XXXXXX" );

    if((fd = mkstemps( temp_name, 0 )) == -1)
    {
        fprintf(stderr, "Could not generate a temp name from %s\n", temp_name);
        exit(2);
    }

    if (!(output = fdopen(fd, "wt")))
    {
        fprintf(stderr,"Could not open fd %s for writing\n", temp_name);
        exit(2);
    }

    *output_name = temp_name;
    ret = wpp_parse( input, output );
    fclose( output );
    return ret;
}
