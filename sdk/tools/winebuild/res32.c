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

typedef unsigned short WCHAR;
typedef unsigned short WORD;
typedef unsigned int DWORD;

/* Unicode string or integer id */
struct string_id
{
    WCHAR *str;  /* ptr to Unicode string */
    WORD   id;   /* integer id if str is NULL */
};

/* descriptor for a resource */
struct resource
{
    struct string_id type;
    struct string_id name;
    const void      *data;
    unsigned int     data_size;
    unsigned int     data_offset;
    unsigned short   mem_options;
    unsigned short   lang;
};

/* name level of the resource tree */
struct res_name
{
    const struct string_id  *name;         /* name */
    struct resource         *res;          /* resource */
    int                      nb_languages; /* number of languages */
    unsigned int             dir_offset;   /* offset of directory in resource dir */
    unsigned int             name_offset;  /* offset of name in resource dir */
};

/* type level of the resource tree */
struct res_type
{
    const struct string_id  *type;         /* type name */
    struct res_name         *names;        /* names array */
    unsigned int             nb_names;     /* total number of names */
    unsigned int             nb_id_names;  /* number of names that have a numeric id */
    unsigned int             dir_offset;   /* offset of directory in resource dir */
    unsigned int             name_offset;  /* offset of type name in resource dir */
};

/* top level of the resource tree */
struct res_tree
{
    struct res_type *types;                /* types array */
    unsigned int     nb_types;             /* total number of types */
};

/* size of a resource directory with n entries */
#define RESOURCE_DIR_SIZE        (4 * sizeof(unsigned int))
#define RESOURCE_DIR_ENTRY_SIZE  (2 * sizeof(unsigned int))
#define RESOURCE_DATA_ENTRY_SIZE (4 * sizeof(unsigned int))
#define RESDIR_SIZE(n)  (RESOURCE_DIR_SIZE + (n) * RESOURCE_DIR_ENTRY_SIZE)


static inline struct resource *add_resource( DLLSPEC *spec )
{
    spec->resources = xrealloc( spec->resources, (spec->nb_resources + 1) * sizeof(spec->resources[0]) );
    return &spec->resources[spec->nb_resources++];
}

static inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline int strcmpW( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static struct res_name *add_name( struct res_type *type, struct resource *res )
{
    struct res_name *name;
    type->names = xrealloc( type->names, (type->nb_names + 1) * sizeof(*type->names) );
    name = &type->names[type->nb_names++];
    name->name         = &res->name;
    name->res          = res;
    name->nb_languages = 1;
    if (!name->name->str) type->nb_id_names++;
    return name;
}

static struct res_type *add_type( struct res_tree *tree, struct resource *res )
{
    struct res_type *type;
    tree->types = xrealloc( tree->types, (tree->nb_types + 1) * sizeof(*tree->types) );
    type = &tree->types[tree->nb_types++];
    type->type        = &res->type;
    type->names       = NULL;
    type->nb_names    = 0;
    type->nb_id_names = 0;
    return type;
}

/* get a string from the current resource file */
static void get_string( struct string_id *str )
{
    WCHAR wc = get_word();

    if (wc == 0xffff)
    {
        str->str = NULL;
        str->id = get_word();
    }
    else
    {
        WCHAR *p = xmalloc( (strlenW( (const WCHAR *)(input_buffer + input_buffer_pos) - 1) + 1) * sizeof(WCHAR) );
        str->str = p;
        str->id  = 0;
        if ((*p++ = wc)) while ((*p++ = get_word()));
    }
}

/* put a string into the resource file */
static void put_string( const struct string_id *str )
{
    if (str->str)
    {
        const WCHAR *p = str->str;
        while (*p) put_word( *p++ );
        put_word( 0 );
    }
    else
    {
        put_word( 0xffff );
        put_word( str->id );
    }
}

static void dump_res_data( const struct resource *res )
{
    unsigned int i = 0;
    unsigned int size = (res->data_size + 3) & ~3;

    if (!size) return;

    input_buffer = res->data;
    input_buffer_pos  = 0;
    input_buffer_size = size;

    output( "\t.long " );
    while (size > 4)
    {
        if ((i++ % 16) == 15) output( "0x%08x\n\t.long ", get_dword() );
        else output( "0x%08x,", get_dword() );
        size -= 4;
    }
    output( "0x%08x\n", get_dword() );
    size -= 4;
    assert( input_buffer_pos == input_buffer_size );
}

/* check the file header */
/* all values must be zero except header size */
static int check_header(void)
{
    DWORD size;

    if (get_dword()) return 0;        /* data size */
    size = get_dword();               /* header size */
    if (size == 0x20000000) byte_swapped = 1;
    else if (size != 0x20) return 0;
    if (get_word() != 0xffff || get_word()) return 0;  /* type, must be id 0 */
    if (get_word() != 0xffff || get_word()) return 0;  /* name, must be id 0 */
    if (get_dword()) return 0;        /* data version */
    if (get_word()) return 0;         /* mem options */
    if (get_word()) return 0;         /* language */
    if (get_dword()) return 0;        /* version */
    if (get_dword()) return 0;        /* characteristics */
    return 1;
}

/* load the next resource from the current file */
static void load_next_resource( DLLSPEC *spec )
{
    DWORD hdr_size;
    struct resource *res = add_resource( spec );

    res->data_size = get_dword();
    hdr_size = get_dword();
    if (hdr_size & 3) fatal_error( "%s header size not aligned\n", input_buffer_filename );

    res->data = input_buffer + input_buffer_pos - 2*sizeof(DWORD) + hdr_size;
    get_string( &res->type );
    get_string( &res->name );
    if (input_buffer_pos & 2) get_word();  /* align to dword boundary */
    get_dword();                        /* skip data version */
    res->mem_options = get_word();
    res->lang = get_word();
    get_dword();                        /* skip version */
    get_dword();                        /* skip characteristics */

    input_buffer_pos = ((const unsigned char *)res->data - input_buffer) + ((res->data_size + 3) & ~3);
    input_buffer_pos = (input_buffer_pos + 3) & ~3;
    if (input_buffer_pos > input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
}

/* load a Win32 .res file */
int load_res32_file( const char *name, DLLSPEC *spec )
{
    int ret;

    init_input_buffer( name );

    if ((ret = check_header()))
    {
        while (input_buffer_pos < input_buffer_size) load_next_resource( spec );
    }
    return ret;
}

/* compare two unicode strings/ids */
static int cmp_string( const struct string_id *str1, const struct string_id *str2 )
{
    if (!str1->str)
    {
        if (!str2->str) return str1->id - str2->id;
        return 1;  /* an id compares larger than a string */
    }
    if (!str2->str) return -1;
    return strcmpW( str1->str, str2->str );
}

/* compare two resources for sorting the resource directory */
/* resources are stored first by type, then by name, then by language */
static int cmp_res( const void *ptr1, const void *ptr2 )
{
    const struct resource *res1 = ptr1;
    const struct resource *res2 = ptr2;
    int ret;

    if ((ret = cmp_string( &res1->type, &res2->type ))) return ret;
    if ((ret = cmp_string( &res1->name, &res2->name ))) return ret;
    return res1->lang - res2->lang;
}

static char *format_res_string( const struct string_id *str )
{
    int i, len = str->str ? strlenW(str->str) + 1 : 5;
    char *ret = xmalloc( len );

    if (!str->str) sprintf( ret, "%04x", str->id );
    else for (i = 0; i < len; i++) ret[i] = str->str[i];  /* dumb W->A conversion */
    return ret;
}

/* build the 3-level (type,name,language) resource tree */
static struct res_tree *build_resource_tree( DLLSPEC *spec, unsigned int *dir_size )
{
    unsigned int i, k, n, offset, data_offset;
    struct res_tree *tree;
    struct res_type *type = NULL;
    struct res_name *name = NULL;
    struct resource *res;

    qsort( spec->resources, spec->nb_resources, sizeof(*spec->resources), cmp_res );

    tree = xmalloc( sizeof(*tree) );
    tree->types = NULL;
    tree->nb_types = 0;

    for (i = 0; i < spec->nb_resources; i++)
    {
        if (!i || cmp_string( &spec->resources[i].type, &spec->resources[i-1].type ))  /* new type */
        {
            type = add_type( tree, &spec->resources[i] );
            name = add_name( type, &spec->resources[i] );
        }
        else if (cmp_string( &spec->resources[i].name, &spec->resources[i-1].name )) /* new name */
        {
            name = add_name( type, &spec->resources[i] );
        }
        else if (spec->resources[i].lang == spec->resources[i-1].lang)
        {
            char *type_str = format_res_string( &spec->resources[i].type );
            char *name_str = format_res_string( &spec->resources[i].name );
            error( "winebuild: duplicate resource type %s name %s language %04x\n",
                   type_str, name_str, spec->resources[i].lang );
        }
        else name->nb_languages++;
    }

    /* compute the offsets */

    offset = RESDIR_SIZE( tree->nb_types );
    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        type->dir_offset = offset;
        offset += RESDIR_SIZE( type->nb_names );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            name->dir_offset = offset;
            offset += RESDIR_SIZE( name->nb_languages );
        }
    }
    data_offset = offset;
    offset += spec->nb_resources * RESOURCE_DATA_ENTRY_SIZE;

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str)
        {
            type->name_offset = offset | 0x80000000;
            offset += (strlenW(type->type->str)+1) * sizeof(WCHAR);
        }
        else type->name_offset = type->type->id;

        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            if (name->name->str)
            {
                name->name_offset = offset | 0x80000000;
                offset += (strlenW(name->name->str)+1) * sizeof(WCHAR);
            }
            else name->name_offset = name->name->id;
            for (k = 0, res = name->res; k < name->nb_languages; k++, res++)
            {
                unsigned int entry_offset = (res - spec->resources) * RESOURCE_DATA_ENTRY_SIZE;
                res->data_offset = data_offset + entry_offset;
            }
        }
    }
    if (dir_size) *dir_size = (offset + 3) & ~3;
    return tree;
}

/* free the resource tree */
static void free_resource_tree( struct res_tree *tree )
{
    unsigned int i;

    for (i = 0; i < tree->nb_types; i++) free( tree->types[i].names );
    free( tree->types );
    free( tree );
}

/* output a Unicode string */
static void output_string( const WCHAR *name )
{
    int i, len = strlenW(name);
    output( "\t%s 0x%04x", get_asm_short_keyword(), len );
    for (i = 0; i < len; i++) output( ",0x%04x", name[i] );
    output( " /* " );
    for (i = 0; i < len; i++) output( "%c", isprint((char)name[i]) ? (char)name[i] : '?' );
    output( " */\n" );
}

/* output a resource directory */
static inline void output_res_dir( unsigned int nb_names, unsigned int nb_ids )
{
    output( "\t.long 0\n" );  /* Characteristics */
    output( "\t.long 0\n" );  /* TimeDateStamp */
    output( "\t%s 0,0\n",     /* Major/MinorVersion */
             get_asm_short_keyword() );
    output( "\t%s %u,%u\n",   /* NumberOfNamed/IdEntries */
             get_asm_short_keyword(), nb_names, nb_ids );
}

/* output the resource definitions */
void output_resources( DLLSPEC *spec )
{
    int k, nb_id_types;
    unsigned int i, n;
    struct res_tree *tree;
    struct res_type *type;
    struct res_name *name;
    const struct resource *res;

    if (!spec->nb_resources) return;

    tree = build_resource_tree( spec, NULL );

    /* output the resource directories */

    output( "\n/* resources */\n\n" );
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( ".L__wine_spec_resources:\n" );

    for (i = nb_id_types = 0, type = tree->types; i < tree->nb_types; i++, type++)
        if (!type->type->str) nb_id_types++;

    output_res_dir( tree->nb_types - nb_id_types, nb_id_types );

    /* dump the type directory */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
        output( "\t.long 0x%08x,0x%08x\n",
                 type->name_offset, type->dir_offset | 0x80000000 );

    /* dump the names and languages directories */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        output_res_dir( type->nb_names - type->nb_id_names, type->nb_id_names );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
            output( "\t.long 0x%08x,0x%08x\n",
                     name->name_offset, name->dir_offset | 0x80000000 );

        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            output_res_dir( 0, name->nb_languages );
            for (k = 0, res = name->res; k < name->nb_languages; k++, res++)
                output( "\t.long 0x%08x,0x%08x\n", res->lang, res->data_offset );
        }
    }

    /* dump the resource data entries */

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
        output( "\t.long .L__wine_spec_res_%d-.L__wine_spec_rva_base,%u,0,0\n",
                i, (res->data_size + 3) & ~3 );

    /* dump the name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str) output_string( type->type->str );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
            if (name->name->str) output_string( name->name->str );
    }

    /* resource data */

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        output( "\n\t.align %d\n", get_alignment(get_ptr_size()) );
        output( ".L__wine_spec_res_%d:\n", i );
        dump_res_data( res );
    }
    output( ".L__wine_spec_resources_end:\n" );
    output( "\t.byte 0\n" );

    free_resource_tree( tree );
}

/* output a Unicode string in binary format */
static void output_bin_string( const WCHAR *name )
{
    int i, len = strlenW(name);
    put_word( len );
    for (i = 0; i < len; i++) put_word( name[i] );
}

/* output a resource directory in binary format */
static inline void output_bin_res_dir( unsigned int nb_names, unsigned int nb_ids )
{
    put_dword( 0 );        /* Characteristics */
    put_dword( 0 );        /* TimeDateStamp */
    put_word( 0 );         /* MajorVersion */
    put_word( 0 );         /* MinorVersion */
    put_word( nb_names );  /* NumberOfNamedEntries */
    put_word( nb_ids );    /* NumberOfIdEntries */
}

/* output the resource definitions in binary format */
void output_bin_resources( DLLSPEC *spec, unsigned int start_rva )
{
    int k, nb_id_types;
    unsigned int i, n, data_offset;
    struct res_tree *tree;
    struct res_type *type;
    struct res_name *name;
    const struct resource *res;

    if (!spec->nb_resources) return;

    tree = build_resource_tree( spec, &data_offset );
    init_output_buffer();

    /* output the resource directories */

    for (i = nb_id_types = 0, type = tree->types; i < tree->nb_types; i++, type++)
        if (!type->type->str) nb_id_types++;

    output_bin_res_dir( tree->nb_types - nb_id_types, nb_id_types );

    /* dump the type directory */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        put_dword( type->name_offset );
        put_dword( type->dir_offset | 0x80000000 );
    }

    /* dump the names and languages directories */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        output_bin_res_dir( type->nb_names - type->nb_id_names, type->nb_id_names );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            put_dword( name->name_offset );
            put_dword( name->dir_offset | 0x80000000 );
        }

        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            output_bin_res_dir( 0, name->nb_languages );
            for (k = 0, res = name->res; k < name->nb_languages; k++, res++)
            {
                put_dword( res->lang );
                put_dword( res->data_offset );
            }
        }
    }

    /* dump the resource data entries */

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        put_dword( data_offset + start_rva );
        put_dword( (res->data_size + 3) & ~3 );
        put_dword( 0 );
        put_dword( 0 );
        data_offset += (res->data_size + 3) & ~3;
    }

    /* dump the name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str) output_bin_string( type->type->str );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
            if (name->name->str) output_bin_string( name->name->str );
    }

    /* resource data */

    align_output( 4 );
    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        put_data( res->data, res->data_size );
        align_output( 4 );
    }

    free_resource_tree( tree );
}

static unsigned int get_resource_header_size( const struct resource *res )
{
    unsigned int size  = 5 * sizeof(unsigned int) + 2 * sizeof(unsigned short);

    if (!res->type.str) size += 2 * sizeof(unsigned short);
    else size += (strlenW(res->type.str) + 1) * sizeof(WCHAR);

    if (!res->name.str) size += 2 * sizeof(unsigned short);
    else size += (strlenW(res->name.str) + 1) * sizeof(WCHAR);

    return size;
}

/* output the resources into a .o file */
void output_res_o_file( DLLSPEC *spec )
{
    unsigned int i;
    char *res_file = NULL;
    int fd, err;

    if (!spec->nb_resources) fatal_error( "--resources mode needs at least one resource file as input\n" );
    if (!output_file_name) fatal_error( "No output file name specified\n" );

    byte_swapped = 0;
    init_output_buffer();

    put_dword( 0 );      /* ResSize */
    put_dword( 32 );     /* HeaderSize */
    put_word( 0xffff );  /* ResType */
    put_word( 0x0000 );
    put_word( 0xffff );  /* ResName */
    put_word( 0x0000 );
    put_dword( 0 );      /* DataVersion */
    put_word( 0 );       /* Memory options */
    put_word( 0 );       /* Language */
    put_dword( 0 );      /* Version */
    put_dword( 0 );      /* Characteristics */

    for (i = 0; i < spec->nb_resources; i++)
    {
        unsigned int header_size = get_resource_header_size( &spec->resources[i] );

        put_dword( spec->resources[i].data_size );
        put_dword( (header_size + 3) & ~3 );
        put_string( &spec->resources[i].type );
        put_string( &spec->resources[i].name );
        align_output( 4 );
        put_dword( 0 );
        put_word( spec->resources[i].mem_options );
        put_word( spec->resources[i].lang );
        put_dword( 0 );
        put_dword( 0 );
        put_data( spec->resources[i].data, spec->resources[i].data_size );
        align_output( 4 );
    }

    /* if the output file name is a .res too, don't run the results through windres */
    if (strendswith( output_file_name, ".res"))
    {
        flush_output_buffer();
        return;
    }

    res_file = get_temp_file_name( output_file_name, ".res" );
    if ((fd = open( res_file, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0600 )) == -1)
        fatal_error( "Cannot create %s\n", res_file );
    if (write( fd, output_buffer, output_buffer_pos ) != output_buffer_pos)
        fatal_error( "Error writing to %s\n", res_file );
    close( fd );
    free( output_buffer );

    if (res_file)
    {
        const char *prog = get_windres_command();
        char *cmd = xmalloc( strlen(prog) + strlen(res_file) + strlen(output_file_name) + 9 );
        sprintf( cmd, "%s -i %s -o %s", prog, res_file, output_file_name );
        if (verbose) fprintf( stderr, "%s\n", cmd );
        err = system( cmd );
        if (err) fatal_error( "%s failed with status %d\n", prog, err );
        free( cmd );
    }
    output_file_name = NULL;  /* so we don't try to assemble it */
}
