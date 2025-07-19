/*
 * Exported functions of the Wine preprocessor
 *
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

#ifndef __WINE_WPP_H
#define __WINE_WPP_H

#include <stdio.h>
#include <stdarg.h>

struct wpp_callbacks
{
    /* I/O callbacks */

    /* Looks for a file to include, returning the path where it is found */
    /* The type param is true for local (#include "filename.h") includes */
    /* parent_name is the directory of the parent source file, includepath
     * is an array of additional include paths */
    char *(*lookup)( const char *filename, int type, const char *parent_name,
                     char **include_path, int include_path_count );
    /* Opens an include file */
    void *(*open)( const char *filename, int type );
    /* Closes a previously opened file */
    void (*close)( void *file );
    /* Reads buffer from the input */
    int (*read)( void *file, char *buffer, unsigned int len );
    /* Writes buffer to the output */
    void (*write)( const char *buffer, unsigned int len );

    /* Error callbacks */
    void (*error)( const char *file, int line, int col, const char *near, const char *msg, va_list ap );
    void (*warning)( const char *file, int line, int col, const char *near, const char *msg, va_list ap );
};

/* Return value == 0 means successful execution */
extern int wpp_add_define( const char *name, const char *value );
extern void wpp_del_define( const char *name );
extern int wpp_add_cmdline_define( const char *value );
extern void wpp_set_debug( int lex_debug, int parser_debug, int msg_debug );
extern void wpp_set_pedantic( int on );
extern int wpp_add_include_path( const char *path );
extern char *wpp_find_include( const char *name, const char *parent_name );
extern int wpp_parse( const char *input, FILE *output );
extern void wpp_set_callbacks( const struct wpp_callbacks *callbacks );

#endif  /* __WINE_WPP_H */
