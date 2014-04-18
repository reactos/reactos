/*
 * Utility routines
 *
 * Copyright 1998 Bertho A. Stultiens
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"

#define CURRENT_LOCATION { input_name ? input_name : "stdin", line_number, parser_text }

static const int want_near_indication = 0;

static void make_print(char *str)
{
	while(*str)
	{
		if(!isprint(*str))
			*str = ' ';
		str++;
	}
}

static void generic_msg(const loc_info_t *loc_info, const char *s, const char *t, va_list ap)
{
	fprintf(stderr, "%s:%d: %s: ", loc_info->input_name, loc_info->line_number, t);
	vfprintf(stderr, s, ap);

	if (want_near_indication)
	{
		char *cpy;
		if(loc_info->near_text)
		{
			cpy = xstrdup(loc_info->near_text);
			make_print(cpy);
			fprintf(stderr, " near '%s'", cpy);
			free(cpy);
		}
	}
}


void error_loc(const char *s, ...)
{
	loc_info_t cur_loc = CURRENT_LOCATION;
	va_list ap;
	va_start(ap, s);
	generic_msg(&cur_loc, s, "error", ap);
	va_end(ap);
	exit(1);
}

/* yyerror:  yacc assumes this is not newline terminated.  */
void parser_error(const char *s)
{
	error_loc("%s\n", s);
}

void error_loc_info(const loc_info_t *loc_info, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(loc_info, s, "error", ap);
	va_end(ap);
	exit(1);
}

int parser_warning(const char *s, ...)
{
	loc_info_t cur_loc = CURRENT_LOCATION;
	va_list ap;
	va_start(ap, s);
	generic_msg(&cur_loc, s, "warning", ap);
	va_end(ap);
	return 0;
}

void error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "error: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
	exit(2);
}

void warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
}

void warning_loc_info(const loc_info_t *loc_info, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(loc_info, s, "warning", ap);
	va_end(ap);
}

void chat(const char *s, ...)
{
	if(debuglevel & DEBUGLEVEL_CHAT)
	{
		va_list ap;
		va_start(ap, s);
		fprintf(stderr, "chat: ");
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
		name = "widl.tab";

	slash = strrchr(name, '/');
	if (!slash)
		slash = strrchr(name, '\\');

	if (slash)
		name = slash + 1;

	namelen = strlen(name);

	/* +6 for later extension (strlen("_r.rgs")) and +1 for '\0' */
	base = xmalloc(namelen +6 +1);
	strcpy(base, name);
	if(!strcasecmp(name + namelen-extlen, ext))
	{
		base[namelen - extlen] = '\0';
	}
	return base;
}

size_t widl_getline(char **linep, size_t *lenp, FILE *fp)
{
    char *line = *linep;
    size_t len = *lenp;
    size_t n = 0;

    if (!line)
    {
        len = 64;
        line = xmalloc(len);
    }

    while (fgets(&line[n], len - n, fp))
    {
        n += strlen(&line[n]);
        if (line[n - 1] == '\n')
            break;
        else if (n == len - 1)
        {
            len *= 2;
            line = xrealloc(line, len);
        }
    }

    *linep = line;
    *lenp = len;
    return n;
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
	s = xmalloc(strlen(str)+1);
	return strcpy(s, str);
}

int strendswith(const char* str, const char* end)
{
    int l = strlen(str);
    int m = strlen(end);
    return l >= m && strcmp(str + l - m, end) == 0;
}

/*******************************************************************
 *         buffer management
 *
 * Function for writing to a memory buffer.
 */

int byte_swapped = 0;
unsigned char *output_buffer;
size_t output_buffer_pos;
size_t output_buffer_size;

static struct resource
{
    unsigned char *data;
    size_t         size;
} resources[16];
static unsigned int nb_resources;

static void check_output_buffer_space( size_t size )
{
    if (output_buffer_pos + size >= output_buffer_size)
    {
        output_buffer_size = max( output_buffer_size * 2, output_buffer_pos + size );
        output_buffer = xrealloc( output_buffer, output_buffer_size );
    }
}

void init_output_buffer(void)
{
    output_buffer_size = 1024;
    output_buffer_pos = 0;
    output_buffer = xmalloc( output_buffer_size );
}

void flush_output_buffer( const char *name )
{
    int fd = open( name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666 );
    if (fd == -1) error( "Error creating %s\n", name );
    if (write( fd, output_buffer, output_buffer_pos ) != output_buffer_pos)
        error( "Error writing to %s\n", name );
    close( fd );
    free( output_buffer );
}

static inline void put_resource_id( const char *str )
{
    if (str[0] != '#')
    {
        while (*str)
        {
            unsigned char ch = *str++;
            put_word( toupper(ch) );
        }
        put_word( 0 );
    }
    else
    {
        put_word( 0xffff );
        put_word( atoi( str + 1 ));
    }
}

void add_output_to_resources( const char *type, const char *name )
{
    size_t data_size = output_buffer_pos;
    size_t header_size = 5 * sizeof(unsigned int) + 2 * sizeof(unsigned short);

    assert( nb_resources < sizeof(resources)/sizeof(resources[0]) );

    if (type[0] != '#') header_size += (strlen( type ) + 1) * sizeof(unsigned short);
    else header_size += 2 * sizeof(unsigned short);
    if (name[0] != '#') header_size += (strlen( name ) + 1) * sizeof(unsigned short);
    else header_size += 2 * sizeof(unsigned short);

    header_size = (header_size + 3) & ~3;
    align_output( 4 );
    check_output_buffer_space( header_size );
    resources[nb_resources].size = header_size + output_buffer_pos;
    memmove( output_buffer + header_size, output_buffer, output_buffer_pos );

    output_buffer_pos = 0;
    put_dword( data_size );    /* ResSize */
    put_dword( header_size );  /* HeaderSize */
    put_resource_id( type );   /* ResType */
    put_resource_id( name );   /* ResName */
    align_output( 4 );
    put_dword( 0 );            /* DataVersion */
    put_word( 0 );             /* Memory options */
    put_word( 0 );             /* Language */
    put_dword( 0 );            /* Version */
    put_dword( 0 );            /* Characteristics */

    resources[nb_resources++].data = output_buffer;
    init_output_buffer();
}

void flush_output_resources( const char *name )
{
    int fd;
    unsigned int i;

    /* all output must have been saved with add_output_to_resources() first */
    assert( !output_buffer_pos );

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

    fd = open( name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666 );
    if (fd == -1) error( "Error creating %s\n", name );
    if (write( fd, output_buffer, output_buffer_pos ) != output_buffer_pos)
        error( "Error writing to %s\n", name );
    for (i = 0; i < nb_resources; i++)
    {
        if (write( fd, resources[i].data, resources[i].size ) != resources[i].size)
            error( "Error writing to %s\n", name );
        free( resources[i].data );
    }
    close( fd );
    nb_resources = 0;
    free( output_buffer );
}

void put_data( const void *data, size_t size )
{
    check_output_buffer_space( size );
    memcpy( output_buffer + output_buffer_pos, data, size );
    output_buffer_pos += size;
}

void put_byte( unsigned char val )
{
    check_output_buffer_space( 1 );
    output_buffer[output_buffer_pos++] = val;
}

void put_word( unsigned short val )
{
    if (byte_swapped) val = (val << 8) | (val >> 8);
    put_data( &val, sizeof(val) );
}

void put_dword( unsigned int val )
{
    if (byte_swapped)
        val = ((val << 24) | ((val << 8) & 0x00ff0000) | ((val >> 8) & 0x0000ff00) | (val >> 24));
    put_data( &val, sizeof(val) );
}

void put_qword( unsigned int val )
{
    if (byte_swapped)
    {
        put_dword( 0 );
        put_dword( val );
    }
    else
    {
        put_dword( val );
        put_dword( 0 );
    }
}

/* pointer-sized word */
void put_pword( unsigned int val )
{
    if (pointer_size == 8) put_qword( val );
    else put_dword( val );
}

void put_str( int indent, const char *format, ... )
{
    int n;
    va_list args;

    check_output_buffer_space( 4 * indent );
    memset( output_buffer + output_buffer_pos, ' ', 4 * indent );
    output_buffer_pos += 4 * indent;

    for (;;)
    {
        size_t size = output_buffer_size - output_buffer_pos;
        va_start( args, format );
	n = vsnprintf( (char *)output_buffer + output_buffer_pos, size, format, args );
	va_end( args );
        if (n == -1) size *= 2;
        else if ((size_t)n >= size) size = n + 1;
        else
        {
            output_buffer_pos += n;
            return;
        }
        check_output_buffer_space( size );
    }
}

void align_output( unsigned int align )
{
    size_t size = align - (output_buffer_pos % align);

    if (size == align) return;
    check_output_buffer_space( size );
    memset( output_buffer + output_buffer_pos, 0, size );
    output_buffer_pos += size;
}
