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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"

void error_at( const struct location *where, const char *s, ... )
{
    char buffer[1024];

    va_list ap;
    va_start( ap, s );
    vsnprintf( buffer, sizeof(buffer), s, ap );
    va_end( ap );

    parser_error( where, buffer );
    exit( 1 );
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

void warning_at( const struct location *where, const char *s, ... )
{
    char buffer[1024];

    va_list ap;
    va_start( ap, s );
    vsnprintf( buffer, sizeof(buffer), s, ap );
    va_end( ap );

    parser_warning( where, buffer );
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

size_t strappend(char **buf, size_t *len, size_t pos, const char* fmt, ...)
{
    size_t size;
    va_list ap;
    char *ptr;
    int n;

    assert( buf && len );
    assert( (*len == 0 && *buf == NULL) || (*len != 0 && *buf != NULL) );

    if (*buf)
    {
        size = *len;
        ptr = *buf;
    }
    else
    {
        size = 100;
        ptr = xmalloc( size );
    }

    for (;;)
    {
        va_start( ap, fmt );
        n = vsnprintf( ptr + pos, size - pos, fmt, ap );
        va_end( ap );
        if (n == -1) size *= 2;
        else if (pos + (size_t)n >= size) size = pos + n + 1;
        else break;
        ptr = xrealloc( ptr, size );
    }

    *len = size;
    *buf = ptr;
    return n;
}

/*******************************************************************
 *         buffer management
 *
 * Function for writing to a memory buffer.
 */

unsigned char *output_buffer;
size_t output_buffer_pos;
size_t output_buffer_size;

static struct resource
{
    unsigned char *data;
    size_t         size;
} resources[16];
static unsigned int nb_resources;

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

    assert( nb_resources < ARRAY_SIZE( resources ));

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

    for (i = 0; i < nb_resources; i++)
    {
        put_data( resources[i].data, resources[i].size );
        free( resources[i].data );
    }
    flush_output_buffer( name );
    nb_resources = 0;
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
