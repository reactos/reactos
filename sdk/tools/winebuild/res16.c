/*
 * Builtin dlls resource support
 *
 * Copyright 2000 Alexandre Julliard
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <fcntl.h>

#include "build.h"

/* Unicode string or integer id */
struct string_id
{
    char  *str;  /* ptr to string */
    unsigned short id;   /* integer id if str is NULL */
};

/* descriptor for a resource */
struct resource
{
    struct string_id type;
    struct string_id name;
    const void      *data;
    unsigned int     name_offset;
    unsigned int     data_size;
    unsigned int     memopt;
};

/* type level of the resource tree */
struct res_type
{
    const struct string_id  *type;         /* type name */
    struct resource         *res;          /* first resource of this type */
    unsigned int             name_offset;  /* name offset if string */
    unsigned int             nb_names;     /* total number of names */
};

/* top level of the resource tree */
struct res_tree
{
    struct res_type *types;                /* types array */
    unsigned int     nb_types;             /* total number of types */
};


static inline struct resource *add_resource( DLLSPEC *spec )
{
    spec->resources = xrealloc( spec->resources, (spec->nb_resources + 1) * sizeof(*spec->resources) );
    return &spec->resources[spec->nb_resources++];
}

static struct res_type *add_type( struct res_tree *tree, struct resource *res )
{
    struct res_type *type;
    tree->types = xrealloc( tree->types, (tree->nb_types + 1) * sizeof(*tree->types) );
    type = &tree->types[tree->nb_types++];
    type->type        = &res->type;
    type->res         = res;
    type->nb_names    = 0;
    return type;
}

/* get a string from the current resource file */
static void get_string( struct string_id *str )
{
    unsigned char c = get_byte();

    if (c == 0xff)
    {
        str->str = NULL;
        str->id = get_word();
    }
    else
    {
        str->str = (char *)input_buffer + input_buffer_pos - 1;
        str->id = 0;
        while (get_byte()) /* nothing */;
    }
}

/* load the next resource from the current file */
static void load_next_resource( DLLSPEC *spec )
{
    struct resource *res = add_resource( spec );

    get_string( &res->type );
    get_string( &res->name );
    res->memopt    = get_word();
    res->data_size = get_dword();
    res->data      = input_buffer + input_buffer_pos;
    input_buffer_pos += res->data_size;
    if (input_buffer_pos > input_buffer_size)
        fatal_error( "%s is a truncated/corrupted file\n", input_buffer_filename );
}

/* load a Win16 .res file */
void load_res16_file( const char *name, DLLSPEC *spec )
{
    init_input_buffer( name );
    while (input_buffer_pos < input_buffer_size) load_next_resource( spec );
}

/* compare two strings/ids */
static int cmp_string( const struct string_id *str1, const struct string_id *str2 )
{
    if (!str1->str)
    {
        if (!str2->str) return str1->id - str2->id;
        return 1;  /* an id compares larger than a string */
    }
    if (!str2->str) return -1;
    return strcasecmp( str1->str, str2->str );
}

/* compare two resources for sorting the resource directory */
/* resources are stored first by type, then by name */
static int cmp_res( const void *ptr1, const void *ptr2 )
{
    const struct resource *res1 = ptr1;
    const struct resource *res2 = ptr2;
    int ret;

    if ((ret = cmp_string( &res1->type, &res2->type ))) return ret;
    return cmp_string( &res1->name, &res2->name );
}

/* build the 2-level (type,name) resource tree */
static struct res_tree *build_resource_tree( DLLSPEC *spec )
{
    unsigned int i, j, offset;
    struct res_tree *tree;
    struct res_type *type = NULL;
    struct resource *res;

    qsort( spec->resources, spec->nb_resources, sizeof(*spec->resources), cmp_res );

    offset = 2;  /* alignment */
    tree = xmalloc( sizeof(*tree) );
    tree->types = NULL;
    tree->nb_types = 0;

    for (i = 0; i < spec->nb_resources; i++)
    {
        if (!i || cmp_string( &spec->resources[i].type, &spec->resources[i-1].type ))  /* new type */
        {
            type = add_type( tree, &spec->resources[i] );
            offset += 8;
        }
        type->nb_names++;
        offset += 12;
    }
    offset += 2;  /* terminator */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str)
        {
            type->name_offset = offset;
            offset += strlen(type->type->str) + 1;
        }
        else type->name_offset = type->type->id | 0x8000;

        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
        {
            if (res->name.str)
            {
                res->name_offset = offset;
                offset += strlen(res->name.str) + 1;
            }
            else res->name_offset = res->name.id | 0x8000;
        }
    }
    return tree;
}

/* free the resource tree */
static void free_resource_tree( struct res_tree *tree )
{
    free( tree->types );
    free( tree );
}

/* output a string preceded by its length */
static void output_string( const char *str )
{
    unsigned int i, len = strlen(str);
    output( "\t.byte 0x%02x", len );
    for (i = 0; i < len; i++) output( ",0x%02x", (unsigned char)str[i] );
    output( " /* %s */\n", str );
}

/* output a string preceded by its length in binary format*/
static void output_bin_string( const char *str )
{
    put_byte( strlen(str) );
    while (*str) put_byte( *str++ );
}

/* output the resource data */
void output_res16_data( DLLSPEC *spec )
{
    const struct resource *res;
    unsigned int i;

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        output( ".L__wine_spec_resource_%u:\n", i );
        dump_bytes( res->data, res->data_size );
    }
}

/* output the resource definitions */
void output_res16_directory( DLLSPEC *spec )
{
    unsigned int i, j;
    struct res_tree *tree;
    const struct res_type *type;
    const struct resource *res;

    tree = build_resource_tree( spec );

    output( "\n.L__wine_spec_ne_rsrctab:\n" );
    output( "\t%s 0\n", get_asm_short_keyword() );  /* alignment */

    /* type and name structures */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        output( "\t%s 0x%04x,%u,0,0\n", get_asm_short_keyword(), type->name_offset, type->nb_names );

        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
        {
            output( "\t%s .L__wine_spec_resource_%lu-.L__wine_spec_dos_header,%u\n",
                    get_asm_short_keyword(), (unsigned long)(res - spec->resources), res->data_size );
            output( "\t%s 0x%04x,0x%04x,0,0\n", get_asm_short_keyword(), res->memopt, res->name_offset );
        }
    }
    output( "\t%s 0\n", get_asm_short_keyword() );  /* terminator */

    /* name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str) output_string( type->type->str );
        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
            if (res->name.str) output_string( res->name.str );
    }
    output( "\t.byte 0\n" );  /* names terminator */

    free_resource_tree( tree );
}

/* output the resource data in binary format */
void output_bin_res16_data( DLLSPEC *spec )
{
    const struct resource *res;
    unsigned int i;

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
        put_data( res->data, res->data_size );
}

/* output the resource definitions in binary format */
void output_bin_res16_directory( DLLSPEC *spec, unsigned int data_offset )
{
    unsigned int i, j;
    struct res_tree *tree;
    const struct res_type *type;
    const struct resource *res;

    tree = build_resource_tree( spec );

    put_word( 0 );  /* alignment */

    /* type and name structures */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        put_word( type->name_offset );
        put_word( type->nb_names );
        put_word( 0 );
        put_word( 0 );

        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
        {
            put_word( data_offset );
            put_word( res->data_size );
            put_word( res->memopt );
            put_word( res->name_offset );
            put_word( 0 );
            put_word( 0 );
            data_offset += res->data_size;
        }
    }
    put_word( 0 );  /* terminator */

    /* name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str) output_bin_string( type->type->str );
        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
            if (res->name.str) output_bin_string( res->name.str );
    }
    put_byte( 0 );  /* names terminator */

    free_resource_tree( tree );
}
