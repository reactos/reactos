/*
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

#ifndef __WINE_BUILD_H
#define __WINE_BUILD_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    TYPE_VARIABLE,     /* variable */
    TYPE_PASCAL,       /* pascal function (Win16) */
    TYPE_ABS,          /* absolute value (Win16) */
    TYPE_STUB,         /* unimplemented stub */
    TYPE_STDCALL,      /* stdcall function (Win32) */
    TYPE_CDECL,        /* cdecl function (Win32) */
    TYPE_VARARGS,      /* varargs function (Win32) */
    TYPE_EXTERN,       /* external symbol (Win32) */
    TYPE_NBTYPES
} ORD_TYPE;

typedef enum
{
    SPEC_WIN16,
    SPEC_WIN32
} SPEC_TYPE;

typedef enum
{
    SPEC_MODE_DLL,
    SPEC_MODE_GUIEXE,
    SPEC_MODE_CUIEXE,
    SPEC_MODE_GUIEXE_UNICODE,
    SPEC_MODE_CUIEXE_UNICODE
} SPEC_MODE;

typedef struct
{
    int n_values;
    int *values;
} ORD_VARIABLE;

typedef struct
{
    int  n_args;
    char arg_types[21];
} ORD_FUNCTION;

typedef struct
{
    int value;
} ORD_ABS;

typedef struct
{
    ORD_TYPE    type;
    int         ordinal;
    int         offset;
    int         lineno;
    int         flags;
    char       *name;         /* public name of this function */
    char       *link_name;    /* name of the C symbol to link to */
    char       *export_name;  /* name exported under for noname exports */
    union
    {
        ORD_VARIABLE   var;
        ORD_FUNCTION   func;
        ORD_ABS        abs;
    } u;
} ORDDEF;

/* entry point flags */
#define FLAG_NORELAY   0x01  /* don't use relay debugging for this function */
#define FLAG_NONAME    0x02  /* don't import function by name */
#define FLAG_RET16     0x04  /* function returns a 16-bit value */
#define FLAG_RET64     0x08  /* function returns a 64-bit value */
#define FLAG_I386      0x10  /* function is i386 only */
#define FLAG_REGISTER  0x20  /* use register calling convention */
#define FLAG_INTERRUPT 0x40  /* function is an interrupt handler */
#define FLAG_PRIVATE   0x80  /* function is private (cannot be imported) */

#define FLAG_FORWARD   0x100 /* function is a forwarded name */

  /* Offset of a structure field relative to the start of the struct */
#define STRUCTOFFSET(type,field) ((int)&((type *)0)->field)

  /* Offset of register relative to the start of the CONTEXT struct */
#define CONTEXTOFFSET(reg)  STRUCTOFFSET(CONTEXT86,reg)

  /* Offset of register relative to the start of the STACK16FRAME struct */
#define STACK16OFFSET(reg)  STRUCTOFFSET(STACK16FRAME,reg)

  /* Offset of register relative to the start of the STACK32FRAME struct */
#define STACK32OFFSET(reg)  STRUCTOFFSET(STACK32FRAME,reg)

  /* Offset of the stack pointer relative to %fs:(0) */
#define STACKOFFSET (STRUCTOFFSET(TEB,cur_stack))


#define MAX_ORDINALS  65535

/* global functions */

#ifndef __GNUC__
#define __attribute__(X)
#endif

extern void *xmalloc (size_t size);
extern void *xrealloc (void *ptr, size_t size);
extern char *xstrdup( const char *str );
extern char *strupper(char *s);
extern void fatal_error( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void fatal_perror( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void error( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void warning( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void output_standard_file_header( FILE *outfile );
extern FILE *open_input_file( const char *srcdir, const char *name );
extern void close_input_file( FILE *file );
extern void dump_bytes( FILE *outfile, const unsigned char *data, int len,
                        const char *label, int constant );
extern const char *make_c_identifier( const char *str );
extern int get_alignment(int alignBoundary);

extern void add_import_dll( const char *name, int delay );
extern void add_ignore_symbol( const char *name );
extern void read_undef_symbols( char **argv );
extern int resolve_imports( void );
extern int output_imports( FILE *outfile );
extern int load_res32_file( const char *name );
extern int output_resources( FILE *outfile );
extern void load_res16_file( const char *name );
extern int output_res16_data( FILE *outfile );
extern int output_res16_directory( unsigned char *buffer );
extern void output_dll_init( FILE *outfile, const char *constructor, const char *destructor );
extern int parse_debug_channels( const char *srcdir, const char *filename );

extern void BuildRelays16( FILE *outfile );
extern void BuildRelays32( FILE *outfile );
extern void BuildSpec16File( FILE *outfile );
extern void BuildSpec32File( FILE *outfile );
extern void BuildDef32File( FILE *outfile );
extern void BuildDebugFile( FILE *outfile, const char *srcdir, char **argv );
extern int ParseTopLevel( FILE *file );

/* global variables */

extern int current_line;
extern int nb_entry_points;
extern int nb_names;
extern int Base;
extern int Limit;
extern int DLLHeapSize;
extern int UsePIC;
extern int debugging;
extern int stack_size;
extern int nb_debug_channels;
extern int nb_lib_paths;
extern int nb_errors;
extern int display_warnings;
extern int kill_at;

extern char *owner_name;
extern char *dll_name;
extern char *dll_file_name;
extern const char *init_func;
extern char *input_file_name;
extern const char *output_file_name;
extern char **debug_channels;
extern char **lib_path;

extern ORDDEF *EntryPoints[MAX_ORDINALS];
extern ORDDEF *Ordinals[MAX_ORDINALS];
extern ORDDEF *Names[MAX_ORDINALS];
extern SPEC_MODE SpecMode;
extern SPEC_TYPE SpecType;

#endif  /* __WINE_BUILD_H */
