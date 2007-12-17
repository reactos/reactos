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

typedef struct
{
    int n_values;
    int *values;
} ORD_VARIABLE;

typedef struct
{
    char arg_types[21];
} ORD_FUNCTION;

typedef struct
{
    unsigned short value;
} ORD_ABS;

typedef struct
{
    ORD_TYPE    type;
    int         ordinal;
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

typedef struct
{
    char            *file_name;          /* file name of the dll */
    char            *dll_name;           /* internal name of the dll */
    char            *init_func;          /* initialization routine */
    SPEC_TYPE        type;               /* type of dll (Win16/Win32) */
    int              base;               /* ordinal base */
    int              limit;              /* ordinal limit */
    int              stack_size;         /* exe stack size */
    int              heap_size;          /* exe heap size */
    int              nb_entry_points;    /* number of used entry points */
    int              alloc_entry_points; /* number of allocated entry points */
    int              nb_names;           /* number of entry points with names */
    unsigned int     nb_resources;       /* number of resources */
    int              characteristics;    /* characteristics for the PE header */
    int              subsystem;          /* subsystem id */
    int              subsystem_major;    /* subsystem version major number */
    int              subsystem_minor;    /* subsystem version minor number */
    ORDDEF          *entry_points;       /* dll entry points */
    ORDDEF         **names;              /* array of entry point names (points into entry_points) */
    ORDDEF         **ordinals;           /* array of dll ordinals (points into entry_points) */
    struct resource *resources;          /* array of dll resources (format differs between Win16/Win32) */
} DLLSPEC;

enum target_cpu
{
    CPU_x86, CPU_x86_64, CPU_SPARC, CPU_ALPHA, CPU_POWERPC
};

enum target_platform
{
    PLATFORM_UNSPECIFIED, PLATFORM_APPLE, PLATFORM_SVR4, PLATFORM_WINDOWS
};

extern enum target_cpu target_cpu;
extern enum target_platform target_platform;

/* entry point flags */
#define FLAG_NORELAY   0x01  /* don't use relay debugging for this function */
#define FLAG_NONAME    0x02  /* don't import function by name */
#define FLAG_RET16     0x04  /* function returns a 16-bit value */
#define FLAG_RET64     0x08  /* function returns a 64-bit value */
#define FLAG_I386      0x10  /* function is i386 only */
#define FLAG_REGISTER  0x20  /* use register calling convention */
#define FLAG_PRIVATE   0x40  /* function is private (cannot be imported) */

#define FLAG_FORWARD   0x80  /* function is a forwarded name */
#define FLAG_EXT_LINK  0x100 /* function links to an external symbol */

  /* Offset of a structure field relative to the start of the struct */
#define STRUCTOFFSET(type,field) ((int)&((type *)0)->field)

  /* Offset of register relative to the start of the CONTEXT struct */
#define CONTEXTOFFSET(reg)  STRUCTOFFSET(CONTEXT86,reg)

  /* Offset of register relative to the start of the STACK16FRAME struct */
#define STACK16OFFSET(reg)  STRUCTOFFSET(STACK16FRAME,reg)

  /* Offset of register relative to the start of the STACK32FRAME struct */
#define STACK32OFFSET(reg)  STRUCTOFFSET(STACK32FRAME,reg)

  /* Offset of the stack pointer relative to %fs:(0) */
#define STACKOFFSET (STRUCTOFFSET(TEB,WOW32Reserved))


#define MAX_ORDINALS  65535

/* global functions */

#ifndef __GNUC__
#define __attribute__(X)
#endif

extern void *xmalloc (size_t size);
extern void *xrealloc (void *ptr, size_t size);
extern char *xstrdup( const char *str );
extern char *strupper(char *s);
extern int strendswith(const char* str, const char* end);
extern void fatal_error( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void fatal_perror( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void error( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void warning( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern char *get_temp_file_name( const char *prefix, const char *suffix );
extern void output_standard_file_header( FILE *outfile );
extern FILE *open_input_file( const char *srcdir, const char *name );
extern void close_input_file( FILE *file );
extern void dump_bytes( FILE *outfile, const void *buffer, unsigned int size );
extern int remove_stdcall_decoration( char *name );
extern void assemble_file( const char *src_file, const char *obj_file );
extern DLLSPEC *alloc_dll_spec(void);
extern void free_dll_spec( DLLSPEC *spec );
extern const char *make_c_identifier( const char *str );
extern const char *get_stub_name( const ORDDEF *odp, const DLLSPEC *spec );
extern unsigned int get_alignment(unsigned int align);
extern unsigned int get_page_size(void);
extern unsigned int get_ptr_size(void);
extern const char *asm_name( const char *func );
extern const char *func_declaration( const char *func );
extern const char *asm_globl( const char *func );
extern const char *get_asm_ptr_keyword(void);
extern const char *get_asm_string_keyword(void);
extern const char *get_asm_short_keyword(void);
extern const char *get_asm_rodata_section(void);
extern const char *get_asm_string_section(void);
extern void output_function_size( FILE *outfile, const char *name );

extern void add_import_dll( const char *name, const char *filename );
extern void add_delayed_import( const char *name );
extern void add_ignore_symbol( const char *name );
extern void add_extra_ld_symbol( const char *name );
extern void read_undef_symbols( DLLSPEC *spec, char **argv );
extern int resolve_imports( DLLSPEC *spec );
extern int has_imports(void);
extern int has_relays( DLLSPEC *spec );
extern void output_get_pc_thunk( FILE *outfile );
extern void output_stubs( FILE *outfile, DLLSPEC *spec );
extern void output_imports( FILE *outfile, DLLSPEC *spec );
extern int load_res32_file( const char *name, DLLSPEC *spec );
extern void output_resources( FILE *outfile, DLLSPEC *spec );
extern void load_res16_file( const char *name, DLLSPEC *spec );
extern void output_res16_data( FILE *outfile, DLLSPEC *spec );
extern void output_res16_directory( FILE *outfile, DLLSPEC *spec, const char *header_name );

extern void BuildRelays16( FILE *outfile );
extern void BuildRelays32( FILE *outfile );
extern void BuildSpec16File( FILE *outfile, DLLSPEC *spec );
extern void BuildSpec32File( FILE *outfile, DLLSPEC *spec );
extern void BuildDef32File( FILE *outfile, DLLSPEC *spec );
extern void BuildPedllFile( FILE *outfile, DLLSPEC *spec );

extern int parse_spec_file( FILE *file, DLLSPEC *spec );
extern int parse_def_file( FILE *file, DLLSPEC *spec );

extern int mkstemps(char *template, int suffix_len);

/* global variables */

extern int current_line;
extern int UsePIC;
extern int nb_lib_paths;
extern int nb_errors;
extern int display_warnings;
extern int kill_at;
extern int verbose;
extern int save_temps;

extern char *input_file_name;
extern char *spec_file_name;
extern const char *output_file_name;
extern char **lib_path;

extern char *as_command;
extern char *ld_command;
extern char *nm_command;

#endif  /* __WINE_BUILD_H */
