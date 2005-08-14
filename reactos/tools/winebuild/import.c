/*
 * DLL imports support
 *
 * Copyright 2000, 2004 Alexandre Julliard
 * Copyright 2000 Eric Pouech
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

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winglue.h"
//#include "wine/exception.h"
#include "build.h"

#ifndef EXCEPTION_NONCONTINUABLE
#define EXCEPTION_NONCONTINUABLE 1
#endif

#define EXCEPTION_WINE_STUB       0x80000100  /* stub entry point called */

struct import
{
    DLLSPEC     *spec;        /* description of the imported dll */
    char        *full_name;   /* full name of the input file */
    dev_t        dev;         /* device/inode of the input file */
    ino_t        ino;
    int          delay;       /* delay or not dll loading ? */
    ORDDEF     **exports;     /* functions exported from this dll */
    int          nb_exports;  /* number of exported functions */
    ORDDEF     **imports;     /* functions we want to import from this dll */
    int          nb_imports;  /* number of imported functions */
};

static char **undef_symbols;  /* list of undefined symbols */
static int nb_undef_symbols = -1;
static int undef_size;

static char **ignore_symbols; /* list of symbols to ignore */
static int nb_ignore_symbols;
static int ignore_size;

static char *ld_tmp_file;  /* ld temp file name */

static struct import **dll_imports = NULL;
static int nb_imports = 0;      /* number of imported dlls (delayed or not) */
static int nb_delayed = 0;      /* number of delayed dlls */
static int total_imports = 0;   /* total number of imported functions */
static int total_delayed = 0;   /* total number of imported functions in delayed DLLs */
static char **delayed_imports;  /* names of delayed import dlls */
static int nb_delayed_imports;  /* size of the delayed_imports array */

/* list of symbols that are ignored by default */
static const char * const default_ignored_symbols[] =
{
    "abs",
    "acos",
    "asin",
    "atan",
    "atan2",
    "atof",
    "atoi",
    "atol",
    "bsearch",
    "ceil",
    "cos",
    "cosh",
    "exp",
    "fabs",
    "floor",
    "fmod",
    "frexp",
    "labs",
    "log",
    "log10",
    "memchr",
    "memcmp",
    "memcpy",
    "memmove",
    "memset",
    "modf",
    "pow",
    "qsort",
    "sin",
    "sinh",
    "sqrt",
    "strcat",
    "strchr",
    "strcmp",
    "strcpy",
    "strcspn",
    "strlen",
    "strncat",
    "strncmp",
    "strncpy",
    "strpbrk",
    "strrchr",
    "strspn",
    "strstr",
    "tan",
    "tanh"
};


static inline const char *ppc_reg( int reg )
{
    static const char * const ppc_regs[32] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                               "r8", "r9", "r10","r11","r12","r13","r14","r15",
                                               "r16","r17","r18","r19","r20","r21","r22","r23",
                                               "r24","r25","r26","r27","r28","r29","r30","r31" };
    if (target_platform == PLATFORM_APPLE) return ppc_regs[reg];
    return ppc_regs[reg] + 1;  /* skip the 'r' */
}

/* compare function names; helper for resolve_imports */
static int name_cmp( const void *name, const void *entry )
{
    return strcmp( *(const char* const *)name, *(const char* const *)entry );
}

/* compare function names; helper for resolve_imports */
static int func_cmp( const void *func1, const void *func2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)func1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)func2;
    return strcmp( odp1->name ? odp1->name : odp1->export_name,
                   odp2->name ? odp2->name : odp2->export_name );
}

/* locate a symbol in a (sorted) list */
inline static const char *find_symbol( const char *name, char **table, int size )
{
    char **res = NULL;

    if (table) {
        res = bsearch( &name, table, size, sizeof(*table), name_cmp );
    }

    return res ? *res : NULL;
}

/* locate an export in a (sorted) export list */
inline static ORDDEF *find_export( const char *name, ORDDEF **table, int size )
{
    ORDDEF func, *odp, **res = NULL;

    func.name = (char *)name;
    func.ordinal = -1;
    odp = &func;
    if (table) res = bsearch( &odp, table, size, sizeof(*table), func_cmp );
    return res ? *res : NULL;
}

/* sort a symbol table */
inline static void sort_symbols( char **table, int size )
{
    if (table )
        qsort( table, size, sizeof(*table), name_cmp );
}

inline static void output_function_size( FILE *outfile, const char *name )
{
    const char *size = func_size( name );
    if (size[0]) fprintf( outfile, "    \"\\t%s\\n\"\n", size );
}

/* free an import structure */
static void free_imports( struct import *imp )
{
    free( imp->exports );
    free( imp->imports );
    free_dll_spec( imp->spec );
    free( imp->full_name );
    free( imp );
}

/* remove the temp file at exit */
static void remove_ld_tmp_file(void)
{
    if (ld_tmp_file) unlink( ld_tmp_file );
}

/* check whether a given dll is imported in delayed mode */
static int is_delayed_import( const char *name )
{
    int i;

    for (i = 0; i < nb_delayed_imports; i++)
    {
        if (!strcmp( delayed_imports[i], name )) return 1;
    }
    return 0;
}

/* check whether a given dll has already been imported */
static struct import *is_already_imported( const char *name )
{
    int i;

    for (i = 0; i < nb_imports; i++)
    {
        if (!strcmp( dll_imports[i]->spec->file_name, name )) return dll_imports[i];
    }
    return NULL;
}

/* open the .so library for a given dll in a specified path */
static char *try_library_path( const char *path, const char *name )
{
    char *buffer;
    int fd;

    buffer = xmalloc( strlen(path) + strlen(name) + 9 );
    sprintf( buffer, "%s/lib%s.def", path, name );

    /* check if the file exists */
    if ((fd = open( buffer, O_RDONLY )) != -1)
    {
        close( fd );
        return buffer;
    }
    free( buffer );
    return NULL;
}

/* find the .def import library for a given dll */
static char *find_library( const char *name )
{
    char *fullname;
    int i;

    for (i = 0; i < nb_lib_paths; i++)
    {
        if ((fullname = try_library_path( lib_path[i], name ))) return fullname;
    }
    fatal_error( "could not open .def file for %s\n", name );
    return NULL;
}

/* read in the list of exported symbols of an import library */
static int read_import_lib( struct import *imp )
{
    FILE *f;
    int i, ret;
    struct stat stat;
    struct import *prev_imp;
    DLLSPEC *spec = imp->spec;

    f = open_input_file( NULL, imp->full_name );
    fstat( fileno(f), &stat );
    imp->dev = stat.st_dev;
    imp->ino = stat.st_ino;
    ret = parse_def_file( f, spec );
    close_input_file( f );
    if (!ret) return 0;

    /* check if we already imported that library from a different file */
    if ((prev_imp = is_already_imported( spec->file_name )))
    {
        if (prev_imp->dev != imp->dev || prev_imp->ino != imp->ino)
            fatal_error( "%s and %s have the same export name '%s'\n",
                         prev_imp->full_name, imp->full_name, spec->file_name );
        return 0;  /* the same file was already loaded, ignore this one */
    }

    if (is_delayed_import( spec->file_name ))
    {
        imp->delay = 1;
        nb_delayed++;
    }

    imp->exports = xmalloc( spec->nb_entry_points * sizeof(*imp->exports) );

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];

        if (odp->type != TYPE_STDCALL && odp->type != TYPE_CDECL) continue;
        if (odp->flags & FLAG_PRIVATE) continue;
        imp->exports[imp->nb_exports++] = odp;
    }
    imp->exports = xrealloc( imp->exports, imp->nb_exports * sizeof(*imp->exports) );
    if (imp->nb_exports)
        qsort( imp->exports, imp->nb_exports, sizeof(*imp->exports), func_cmp );
    return 1;
}

/* build the dll exported name from the import lib name or path */
static char *get_dll_name( const char *name, const char *filename )
{
    char *ret;

    if (filename)
    {
        const char *basename = strrchr( filename, '/' );
        if (!basename) basename = filename;
        else basename++;
        if (!strncmp( basename, "lib", 3 )) basename += 3;
        ret = xmalloc( strlen(basename) + 5 );
        strcpy( ret, basename );
        if (strendswith( ret, ".def" )) ret[strlen(ret)-4] = 0;
    }
    else
    {
        ret = xmalloc( strlen(name) + 5 );
        strcpy( ret, name );
    }
    if (!strchr( ret, '.' )) strcat( ret, ".dll" );
    return ret;
}

/* add a dll to the list of imports */
void add_import_dll( const char *name, const char *filename )
{
    struct import *imp = xmalloc( sizeof(*imp) );

    imp->spec            = alloc_dll_spec();
    imp->spec->file_name = get_dll_name( name, filename );
    imp->delay           = 0;
    imp->imports         = NULL;
    imp->nb_imports      = 0;
    imp->exports         = NULL;
    imp->nb_exports      = 0;

    if (filename) imp->full_name = xstrdup( filename );
    else imp->full_name = find_library( name );

    if (read_import_lib( imp ))
    {
        dll_imports = xrealloc( dll_imports, (nb_imports+1) * sizeof(*dll_imports) );
        dll_imports[nb_imports++] = imp;
    }
    else
    {
        free_imports( imp );
        if (nb_errors) exit(1);
    }
}

/* add a library to the list of delayed imports */
void add_delayed_import( const char *name )
{
    struct import *imp;
    char *fullname = get_dll_name( name, NULL );

    delayed_imports = xrealloc( delayed_imports, (nb_delayed_imports+1) * sizeof(*delayed_imports) );
    delayed_imports[nb_delayed_imports++] = fullname;
    if ((imp = is_already_imported( fullname )) && !imp->delay)
    {
        imp->delay = 1;
        nb_delayed++;
    }
}

/* remove an imported dll, based on its index in the dll_imports array */
static void remove_import_dll( int index )
{
    struct import *imp = dll_imports[index];

    memmove( &dll_imports[index], &dll_imports[index+1], sizeof(imp) * (nb_imports - index - 1) );
    nb_imports--;
    if (imp->delay) nb_delayed--;
    free_imports( imp );
}

/* initialize the list of ignored symbols */
static void init_ignored_symbols(void)
{
    int i;

    nb_ignore_symbols = sizeof(default_ignored_symbols)/sizeof(default_ignored_symbols[0]);
    ignore_size = nb_ignore_symbols + 32;
    ignore_symbols = xmalloc( ignore_size * sizeof(*ignore_symbols) );
    for (i = 0; i < nb_ignore_symbols; i++)
        ignore_symbols[i] = xstrdup( default_ignored_symbols[i] );
}

/* add a symbol to the ignored symbol list */
/* if the name starts with '-' the symbol is removed instead */
void add_ignore_symbol( const char *name )
{
    int i;

    if (!ignore_symbols) init_ignored_symbols();  /* first time around, fill list with defaults */

    if (name[0] == '-')  /* remove it */
    {
        if (!name[1])  /* remove everything */
        {
            for (i = 0; i < nb_ignore_symbols; i++) free( ignore_symbols[i] );
            nb_ignore_symbols = 0;
        }
        else
        {
            for (i = 0; i < nb_ignore_symbols; i++)
            {
                if (!strcmp( ignore_symbols[i], name+1 ))
                {
                    free( ignore_symbols[i] );
                    memmove( &ignore_symbols[i], &ignore_symbols[i+1], nb_ignore_symbols - i - 1 );
                    nb_ignore_symbols--;
                }
            }
        }
    }
    else
    {
        if (nb_ignore_symbols == ignore_size)
        {
            ignore_size += 32;
            ignore_symbols = xrealloc( ignore_symbols, ignore_size * sizeof(*ignore_symbols) );
        }
        ignore_symbols[nb_ignore_symbols++] = xstrdup( name );
    }
}

/* add a function to the list of imports from a given dll */
static void add_import_func( struct import *imp, ORDDEF *func )
{
    imp->imports = xrealloc( imp->imports, (imp->nb_imports+1) * sizeof(*imp->imports) );
    imp->imports[imp->nb_imports++] = func;
    total_imports++;
    if (imp->delay) total_delayed++;
}

/* add a symbol to the undef list */
inline static void add_undef_symbol( const char *name )
{
    if (nb_undef_symbols == undef_size)
    {
        undef_size += 128;
        undef_symbols = xrealloc( undef_symbols, undef_size * sizeof(*undef_symbols) );
    }
    undef_symbols[nb_undef_symbols++] = xstrdup( name );
}

/* remove all the holes in the undefined symbol list; return the number of removed symbols */
static int remove_symbol_holes(void)
{
    int i, off;
    for (i = off = 0; i < nb_undef_symbols; i++)
    {
        if (!undef_symbols[i]) off++;
        else undef_symbols[i - off] = undef_symbols[i];
    }
    nb_undef_symbols -= off;
    return off;
}

/* add a symbol to the extra list, but only if needed */
static int add_extra_symbol( const char **extras, int *count, const char *name, const DLLSPEC *spec )
{
    int i;

    if (!find_symbol( name, undef_symbols, nb_undef_symbols ))
    {
        /* check if the symbol is being exported by this dll */
        for (i = 0; i < spec->nb_entry_points; i++)
        {
            ORDDEF *odp = &spec->entry_points[i];
            if (odp->type == TYPE_STDCALL ||
                odp->type == TYPE_CDECL ||
                odp->type == TYPE_VARARGS ||
                odp->type == TYPE_EXTERN)
            {
                if (odp->name && !strcmp( odp->name, name )) return 0;
            }
        }
        extras[*count] = name;
        (*count)++;
    }
    return 1;
}

/* add the extra undefined symbols that will be contained in the generated spec file itself */
static void add_extra_undef_symbols( const DLLSPEC *spec )
{
    const char *extras[10];
    int i, count = 0, nb_stubs = 0;
    int kernel_imports = 0, ntdll_imports = 0;

    sort_symbols( undef_symbols, nb_undef_symbols );

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB) nb_stubs++;
    }

    /* add symbols that will be contained in the spec file itself */
    if (!(spec->characteristics & IMAGE_FILE_DLL))
    {
        switch (spec->subsystem)
        {
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:
            kernel_imports += add_extra_symbol( extras, &count, "GetCommandLineA", spec );
            kernel_imports += add_extra_symbol( extras, &count, "GetStartupInfoA", spec );
            kernel_imports += add_extra_symbol( extras, &count, "GetModuleHandleA", spec );
            kernel_imports += add_extra_symbol( extras, &count, "ExitProcess", spec );
            break;
        }
    }
    if (nb_delayed)
    {
        kernel_imports += add_extra_symbol( extras, &count, "LoadLibraryA", spec );
        kernel_imports += add_extra_symbol( extras, &count, "FreeLibrary", spec );
        kernel_imports += add_extra_symbol( extras, &count, "GetProcAddress", spec );
        kernel_imports += add_extra_symbol( extras, &count, "RaiseException", spec );
    }
    if (nb_stubs)
        ntdll_imports += add_extra_symbol( extras, &count, "RtlRaiseException", spec );

    /* make sure we import the dlls that contain these functions */
    if (kernel_imports) add_import_dll( "kernel32", NULL );
    if (ntdll_imports) add_import_dll( "ntdll", NULL );

    if (count)
    {
        for (i = 0; i < count; i++) add_undef_symbol( extras[i] );
        sort_symbols( undef_symbols, nb_undef_symbols );
    }
}

/* check if a given imported dll is not needed, taking forwards into account */
static int check_unused( const struct import* imp, const DLLSPEC *spec )
{
    int i;
    const char *file_name = imp->spec->file_name;
    size_t len = strlen( file_name );
    const char *p = strchr( file_name, '.' );
    if (p && !strcasecmp( p, ".dll" )) len = p - file_name;

    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !(odp->flags & FLAG_FORWARD)) continue;
        if (!strncasecmp( odp->link_name, file_name, len ) &&
            odp->link_name[len] == '.')
            return 0;  /* found a forward, it is used */
    }
    return 1;
}

/* combine a list of object files with ld into a single object file */
/* returns the name of the combined file */
static const char *ldcombine_files( char **argv )
{
    int i, len = 0;
    char *cmd;
    int fd, err;

    if (output_file_name && output_file_name[0])
    {
        ld_tmp_file = xmalloc( strlen(output_file_name) + 10 );
        strcpy( ld_tmp_file, output_file_name );
        strcat( ld_tmp_file, ".XXXXXX.o" );
    }
    else ld_tmp_file = xstrdup( "/tmp/winebuild.tmp.XXXXXX.o" );

    if ((fd = mkstemps( ld_tmp_file, 2 ) == -1)) fatal_error( "could not generate a temp file\n" );
    close( fd );
    atexit( remove_ld_tmp_file );

    for (i = 0; argv[i]; i++) len += strlen(argv[i]) + 1;
    cmd = xmalloc( len + strlen(ld_tmp_file) + 8 + strlen(ld_command)  );
    sprintf( cmd, "%s -r -o %s", ld_command, ld_tmp_file );
    for (i = 0; argv[i]; i++) sprintf( cmd + strlen(cmd), " %s", argv[i] );
    err = system( cmd );
    if (err) fatal_error( "%s -r failed with status %d\n", ld_command, err );
    free( cmd );
    return ld_tmp_file;
}

/* read in the list of undefined symbols */
void read_undef_symbols( char **argv )
{
    size_t prefix_len;
    FILE *f;
    char *cmd, buffer[1024], name_prefix[16];
    int err;
    const char *name;

    if (!argv[0]) return;

    strcpy( name_prefix, asm_name("") );
    prefix_len = strlen( name_prefix );

    undef_size = nb_undef_symbols = 0;

    /* if we have multiple object files, link them together */
    if (argv[1]) name = ldcombine_files( argv );
    else name = argv[0];

    cmd = xmalloc( strlen(nm_command) + strlen(name) + 5 );
    sprintf( cmd, "%s -u %s", nm_command, name );
    if (!(f = popen( cmd, "r" )))
        fatal_error( "Cannot execute '%s'\n", cmd );

    while (fgets( buffer, sizeof(buffer), f ))
    {
        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) continue;
        if (*p == '\n') *p-- = 0;
        p = buffer;
        while (*p == ' ') p++;
        if (p[0] == 'U' && p[1] == ' ' && p[2]) p += 2;
        if (prefix_len && !strncmp( p, name_prefix, prefix_len )) p += prefix_len;
        add_undef_symbol( p );
    }
    if ((err = pclose( f ))) warning( "%s failed with status %d\n", cmd, err );
    free( cmd );
}

static void remove_ignored_symbols(void)
{
    int i;

    if (!ignore_symbols) init_ignored_symbols();
    sort_symbols( ignore_symbols, nb_ignore_symbols );
    for (i = 0; i < nb_undef_symbols; i++)
    {
        if (find_symbol( undef_symbols[i], ignore_symbols, nb_ignore_symbols ))
        {
            free( undef_symbols[i] );
            undef_symbols[i] = NULL;
        }
    }
    remove_symbol_holes();
}

/* resolve the imports for a Win32 module */
int resolve_imports( DLLSPEC *spec )
{
    int i, j;

    if (nb_undef_symbols == -1) return 0; /* no symbol file specified */

    add_extra_undef_symbols( spec );
    remove_ignored_symbols();

    for (i = 0; i < nb_imports; i++)
    {
        struct import *imp = dll_imports[i];

        for (j = 0; j < nb_undef_symbols; j++)
        {
            ORDDEF *odp = find_export( undef_symbols[j], imp->exports, imp->nb_exports );
            if (odp)
            {
                add_import_func( imp, odp );
                free( undef_symbols[j] );
                undef_symbols[j] = NULL;
            }
        }
        /* remove all the holes in the undef symbols list */
        if (!remove_symbol_holes() && check_unused( imp, spec ))
        {
            /* the dll is not used, get rid of it */
            warning( "%s imported but no symbols used\n", imp->spec->file_name );
            remove_import_dll( i );
            i--;
        }
    }
    return 1;
}

/* output a single import thunk */
static void output_import_thunk( FILE *outfile, const char *name, const char *table, int pos )
{
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"\\t%s\\n\"\n", func_declaration(name) );
    fprintf( outfile, "    \"\\t.globl %s\\n\"\n", asm_name(name) );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name(name) );

    switch(target_cpu)
    {
    case CPU_x86:
        if (!UsePIC)
        {
            if (strstr( name, "__wine_call_from_16" )) fprintf( outfile, "    \"\\t.byte 0x2e\\n\"\n" );
            fprintf( outfile, "    \"\\tjmp *(imports+%d)\\n\"\n", pos );
        }
        else
        {
            if (!strcmp( name, "__wine_call_from_32_regs" ) ||
                !strcmp( name, "__wine_call_from_16_regs" ))
            {
                /* special case: need to preserve all registers */
                fprintf( outfile, "    \"\\tpushl %%eax\\n\"\n" );
                fprintf( outfile, "    \"\\tpushfl\\n\"\n" );
                fprintf( outfile, "    \"\\tcall .L__wine_spec_%s\\n\"\n", name );
                fprintf( outfile, "    \".L__wine_spec_%s:\\n\"\n", name );
                fprintf( outfile, "    \"\\tpopl %%eax\\n\"\n" );
                fprintf( outfile, "    \"\\taddl $%d+%s-.L__wine_spec_%s,%%eax\\n\"\n",
                         pos, asm_name(table), name );
                if (!strcmp( name, "__wine_call_from_16_regs" ))
                    fprintf( outfile, "    \"\\t.byte 0x2e\\n\"\n" );
                fprintf( outfile, "    \"\\tmovl 0(%%eax),%%eax\\n\"\n" );
                fprintf( outfile, "    \"\\txchgl 4(%%esp),%%eax\\n\"\n" );
                fprintf( outfile, "    \"\\tpopfl\\n\"\n" );
                fprintf( outfile, "    \"\\tret\\n\"\n" );
            }
            else
            {
                fprintf( outfile, "    \"\\tcall .L__wine_spec_%s\\n\"\n", name );
                fprintf( outfile, "    \".L__wine_spec_%s:\\n\"\n", name );
                fprintf( outfile, "    \"\\tpopl %%eax\\n\"\n" );
                fprintf( outfile, "    \"\\taddl $%d+%s-.L__wine_spec_%s,%%eax\\n\"\n",
                         pos, asm_name(table), name );
                if (strstr( name, "__wine_call_from_16" ))
                    fprintf( outfile, "    \"\\t.byte 0x2e\\n\"\n" );
                fprintf( outfile, "    \"\\tjmp *0(%%eax)\\n\"\n" );
            }
        }
        break;
    case CPU_SPARC:
        if ( !UsePIC )
        {
            fprintf( outfile, "    \"\\tsethi %%hi(%s+%d), %%g1\\n\"\n", table, pos );
            fprintf( outfile, "    \"\\tld [%%g1+%%lo(%s+%d)], %%g1\\n\"\n", table, pos );
            fprintf( outfile, "    \"\\tjmp %%g1\\n\\tnop\\n\"\n" );
        }
        else
        {
            /* Hmpf.  Stupid sparc assembler always interprets global variable
               names as GOT offsets, so we have to do it the long way ... */
            fprintf( outfile, "    \"\\tsave %%sp, -96, %%sp\\n\"\n" );
            fprintf( outfile, "    \"0:\\tcall 1f\\n\\tnop\\n\"\n" );
            fprintf( outfile, "    \"1:\\tsethi %%hi(%s+%d-0b), %%g1\\n\"\n", table, pos );
            fprintf( outfile, "    \"\\tor %%g1, %%lo(%s+%d-0b), %%g1\\n\"\n", table, pos );
            fprintf( outfile, "    \"\\tld [%%g1+%%o7], %%g1\\n\"\n" );
            fprintf( outfile, "    \"\\tjmp %%g1\\n\\trestore\\n\"\n" );
        }
        break;
    case CPU_ALPHA:
        fprintf( outfile, "    \"\\tlda $0,%s\\n\"\n", table );
        fprintf( outfile, "    \"\\tlda $0,%d($0)\\n\"\n", pos);
        fprintf( outfile, "    \"\\tjmp $31,($0)\\n\"\n" );
        break;
    case CPU_POWERPC:
        fprintf(outfile, "    \"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
        fprintf(outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg(9), ppc_reg(1));
        fprintf(outfile, "    \"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
        fprintf(outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg(8), ppc_reg(1));
        fprintf(outfile, "    \"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
        fprintf(outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg(7), ppc_reg(1));
        if (target_platform == PLATFORM_APPLE)
        {
            fprintf(outfile, "    \"\\tlis %s, ha16(%s+%d)\\n\"\n",
                    ppc_reg(9), asm_name(table), pos);
            fprintf(outfile, "    \"\\tla  %s, lo16(%s+%d)(%s)\\n\"\n",
                    ppc_reg(8), asm_name(table), pos, ppc_reg(9));
        }
        else
        {
            fprintf(outfile, "    \"\\tlis %s, (%s+%d)@hi\\n\"\n",
                    ppc_reg(9), asm_name(table), pos);
            fprintf(outfile, "    \"\\tla  %s, (%s+%d)@l(%s)\\n\"\n",
                    ppc_reg(8), asm_name(table), pos, ppc_reg(9));
        }
        fprintf(outfile, "    \"\\tlwz  %s, 0(%s)\\n\"\n", ppc_reg(7), ppc_reg(8));
        fprintf(outfile, "    \"\\tmtctr %s\\n\"\n", ppc_reg(7));
        fprintf(outfile, "    \"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg(7), ppc_reg(1));
        fprintf(outfile, "    \"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
        fprintf(outfile, "    \"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg(8), ppc_reg(1));
        fprintf(outfile, "    \"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
        fprintf(outfile, "    \"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg(9), ppc_reg(1));
        fprintf(outfile, "    \"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
        fprintf(outfile, "    \"\\tbctr\\n\"\n");
        break;
    }
    output_function_size( outfile, name );
}

/* output the import table of a Win32 module */
static int output_immediate_imports( FILE *outfile )
{
    int i, j, nb_imm = nb_imports - nb_delayed;

    if (!nb_imm) return 0;

    /* main import header */

    fprintf( outfile, "\nstatic struct {\n" );
    fprintf( outfile, "  struct {\n" );
    fprintf( outfile, "    void        *OriginalFirstThunk;\n" );
    fprintf( outfile, "    unsigned int TimeDateStamp;\n" );
    fprintf( outfile, "    unsigned int ForwarderChain;\n" );
    fprintf( outfile, "    const char  *Name;\n" );
    fprintf( outfile, "    void        *FirstThunk;\n" );
    fprintf( outfile, "  } imp[%d];\n", nb_imm+1 );
    fprintf( outfile, "  const char *data[%d];\n",
             total_imports - total_delayed + nb_imm );
    fprintf( outfile, "} imports = {\n  {\n" );

    /* list of dlls */

    for (i = j = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        fprintf( outfile, "    { 0, 0, 0, \"%s\", &imports.data[%d] },\n",
                 dll_imports[i]->spec->file_name, j );
        j += dll_imports[i]->nb_imports + 1;
    }

    fprintf( outfile, "    { 0, 0, 0, 0, 0 },\n" );
    fprintf( outfile, "  },\n  {\n" );

    /* list of imported functions */

    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->spec->file_name );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            if (!(odp->flags & FLAG_NONAME))
            {
                unsigned short ord = odp->ordinal;
                fprintf( outfile, "    \"\\%03o\\%03o%s\",\n",
                         *(unsigned char *)&ord, *((unsigned char *)&ord + 1), odp->name );
            }
            else
                fprintf( outfile, "    (char *)%d,\n", odp->ordinal );
        }
        fprintf( outfile, "    0,\n" );
    }
    fprintf( outfile, "  }\n};\n\n" );

    return nb_imm;
}

/* output the import thunks of a Win32 module */
static void output_immediate_import_thunks( FILE *outfile )
{
    int i, j, pos;
    int nb_imm = nb_imports - nb_delayed;
    static const char import_thunks[] = "__wine_spec_import_thunks";

    if (!nb_imm) return;

    pos = (sizeof(void *) + 2*sizeof(unsigned int) + sizeof(const char *) + sizeof(void *)) *
            (nb_imm + 1);  /* offset of imports.data from start of imports */
    fprintf( outfile, "/* immediate import thunks */\n" );
    fprintf( outfile, "asm(\".text\\n\\t.align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name(import_thunks));

    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += sizeof(const char *))
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            output_import_thunk( outfile, odp->name ? odp->name : odp->export_name,
                                 "imports", pos );
        }
        pos += 4;
    }
    output_function_size( outfile, import_thunks );
    fprintf( outfile, ");\n" );
}

/* output the delayed import table of a Win32 module */
static int output_delayed_imports( FILE *outfile, const DLLSPEC *spec )
{
    int i, j;

    if (!nb_delayed) return 0;

    fprintf( outfile, "static void *__wine_delay_imp_hmod[%d];\n", nb_delayed );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;
            fprintf( outfile, "void __wine_delay_imp_%d_%s();\n", i, name );
        }
    }
    fprintf( outfile, "\n" );
    fprintf( outfile, "static struct {\n" );
    fprintf( outfile, "  struct ImgDelayDescr {\n" );
    fprintf( outfile, "    unsigned int  grAttrs;\n" );
    fprintf( outfile, "    const char   *szName;\n" );
    fprintf( outfile, "    void        **phmod;\n" );
    fprintf( outfile, "    void        **pIAT;\n" );
    fprintf( outfile, "    const char  **pINT;\n" );
    fprintf( outfile, "    void*         pBoundIAT;\n" );
    fprintf( outfile, "    void*         pUnloadIAT;\n" );
    fprintf( outfile, "    unsigned long dwTimeStamp;\n" );
    fprintf( outfile, "  } imp[%d];\n", nb_delayed );
    fprintf( outfile, "  void         *IAT[%d];\n", total_delayed );
    fprintf( outfile, "  const char   *INT[%d];\n", total_delayed );
    fprintf( outfile, "} delay_imports = {\n" );
    fprintf( outfile, "  {\n" );
    for (i = j = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "    { 0, \"%s\", &__wine_delay_imp_hmod[%d], &delay_imports.IAT[%d], &delay_imports.INT[%d], 0, 0, 0 },\n",
                 dll_imports[i]->spec->file_name, i, j, j );
        j += dll_imports[i]->nb_imports;
    }
    fprintf( outfile, "  },\n  {\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->spec->file_name );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;
            fprintf( outfile, "    &__wine_delay_imp_%d_%s,\n", i, name );
        }
    }
    fprintf( outfile, "  },\n  {\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->spec->file_name );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            if (!odp->name)
                fprintf( outfile, "    (char *)%d,\n", odp->ordinal );
            else
                fprintf( outfile, "    \"%s\",\n", odp->name );
        }
    }
    fprintf( outfile, "  }\n};\n\n" );

    fprintf( outfile, "extern void __stdcall RaiseException(unsigned int, unsigned int, unsigned int, const void *args[]);\n" );
    fprintf( outfile, "extern void * __stdcall LoadLibraryA(const char*);\n");
    fprintf( outfile, "extern void * __stdcall GetProcAddress(void *, const char*);\n");
    fprintf( outfile, "\n" );

    fprintf( outfile, "void *__stdcall __wine_delay_load( int idx_nr )\n" );
    fprintf( outfile, "{\n" );
    fprintf( outfile, "  int idx = idx_nr >> 16, nr = idx_nr & 0xffff;\n" );
    fprintf( outfile, "  struct ImgDelayDescr *imd = delay_imports.imp + idx;\n" );
    fprintf( outfile, "  void **pIAT = imd->pIAT + nr;\n" );
    fprintf( outfile, "  const char** pINT = imd->pINT + nr;\n" );
    fprintf( outfile, "  void *fn;\n\n" );

    fprintf( outfile, "  if (!*imd->phmod) *imd->phmod = LoadLibraryA(imd->szName);\n" );
    fprintf( outfile, "  if (*imd->phmod && (fn = GetProcAddress(*imd->phmod, *pINT)))\n");
    fprintf( outfile, "    /* patch IAT with final value */\n" );
    fprintf( outfile, "    return *pIAT = fn;\n" );
    fprintf( outfile, "  else {\n");
    fprintf( outfile, "    const void *args[2];\n" );
    fprintf( outfile, "    args[0] = imd->szName;\n" );
    fprintf( outfile, "    args[1] = *pINT;\n" );
    fprintf( outfile, "    RaiseException( 0x%08x, %d, 2, args );\n",
             EXCEPTION_WINE_STUB, EXCEPTION_NONCONTINUABLE );
    fprintf( outfile, "    return 0;\n" );
    fprintf( outfile, "  }\n}\n" );

    return nb_delayed;
}

/* output the delayed import thunks of a Win32 module */
static void output_delayed_import_thunks( FILE *outfile, const DLLSPEC *spec )
{
    int i, idx, j, pos, extra_stack_storage = 0;
    static const char delayed_import_loaders[] = "__wine_spec_delayed_import_loaders";
    static const char delayed_import_thunks[] = "__wine_spec_delayed_import_thunks";

    if (!nb_delayed) return;

    fprintf( outfile, "/* delayed import thunks */\n" );
    fprintf( outfile, "asm(\".text\\n\"\n" );
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name(delayed_import_loaders));
    fprintf( outfile, "    \"\\t%s\\n\"\n", func_declaration("__wine_delay_load_asm") );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name("__wine_delay_load_asm") );
    switch(target_cpu)
    {
    case CPU_x86:
        fprintf( outfile, "    \"\\tpushl %%ecx\\n\\tpushl %%edx\\n\\tpushl %%eax\\n\"\n" );
        fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name("__wine_delay_load") );
        fprintf( outfile, "    \"\\tpopl %%edx\\n\\tpopl %%ecx\\n\\tjmp *%%eax\\n\"\n" );
        break;
    case CPU_SPARC:
        fprintf( outfile, "    \"\\tsave %%sp, -96, %%sp\\n\"\n" );
        fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name("__wine_delay_load") );
        fprintf( outfile, "    \"\\tmov %%g1, %%o0\\n\"\n" );
        fprintf( outfile, "    \"\\tjmp %%o0\\n\\trestore\\n\"\n" );
        break;
    case CPU_ALPHA:
        fprintf( outfile, "    \"\\tjsr $26,%s\\n\"\n", asm_name("__wine_delay_load") );
        fprintf( outfile, "    \"\\tjmp $31,($0)\\n\"\n" );
        break;
    case CPU_POWERPC:
        if (target_platform == PLATFORM_APPLE) extra_stack_storage = 56;

        /* Save all callee saved registers into a stackframe. */
        fprintf( outfile, "    \"\\tstwu %s, -%d(%s)\\n\"\n",ppc_reg(1), 48+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(3),  4+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(4),  8+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(5), 12+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(6), 16+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(7), 20+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(8), 24+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(9), 28+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(10),32+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(11),36+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(12),40+extra_stack_storage, ppc_reg(1));

        /* r0 -> r3 (arg1) */
        fprintf( outfile, "    \"\\tmr %s, %s\\n\"\n", ppc_reg(3), ppc_reg(0));

        /* save return address */
        fprintf( outfile, "    \"\\tmflr %s\\n\"\n", ppc_reg(0));
        fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg(0), 44+extra_stack_storage, ppc_reg(1));

        /* Call the __wine_delay_load function, arg1 is arg1. */
        fprintf( outfile, "    \"\\tbl %s\\n\"\n", asm_name("__wine_delay_load") );

        /* Load return value from call into ctr register */
        fprintf( outfile, "    \"\\tmtctr %s\\n\"\n", ppc_reg(3));

        /* restore all saved registers and drop stackframe. */
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(3),  4+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(4),  8+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(5), 12+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(6), 16+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(7), 20+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(8), 24+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(9), 28+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(10),32+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(11),36+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg(12),40+extra_stack_storage, ppc_reg(1));

        /* Load return value from call into return register */
        fprintf( outfile, "    \"\\tlwz  %s,  %d(%s)\\n\"\n", ppc_reg(0), 44+extra_stack_storage, ppc_reg(1));
        fprintf( outfile, "    \"\\tmtlr %s\\n\"\n", ppc_reg(0));
        fprintf( outfile, "    \"\\taddi %s, %s, %d\\n\"\n", ppc_reg(1), ppc_reg(1),  48+extra_stack_storage);

        /* branch to ctr register. */
        fprintf( outfile, "    \"bctr\\n\"\n");
        break;
    }
    output_function_size( outfile, "__wine_delay_load_asm" );

    for (i = idx = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            char buffer[128];
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;

            sprintf( buffer, "__wine_delay_imp_%d_%s", i, name );
            fprintf( outfile, "    \"\\t%s\\n\"\n", func_declaration(buffer) );
            fprintf( outfile, "    \"%s:\\n\"\n", asm_name(buffer) );
            switch(target_cpu)
            {
            case CPU_x86:
                fprintf( outfile, "    \"\\tmovl $%d, %%eax\\n\"\n", (idx << 16) | j );
                fprintf( outfile, "    \"\\tjmp %s\\n\"\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_SPARC:
                fprintf( outfile, "    \"\\tset %d, %%g1\\n\"\n", (idx << 16) | j );
                fprintf( outfile, "    \"\\tb,a %s\\n\"\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_ALPHA:
                fprintf( outfile, "    \"\\tlda $0,%d($31)\\n\"\n", j);
                fprintf( outfile, "    \"\\tldah $0,%d($0)\\n\"\n", idx);
                fprintf( outfile, "    \"\\tjmp $31,%s\\n\"\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_POWERPC:
                switch(target_platform)
                {
                case PLATFORM_APPLE:
                    /* On Darwin we can use r0 and r2 */
                    /* Upper part in r2 */
                    fprintf( outfile, "    \"\\tlis %s, %d\\n\"\n", ppc_reg(2), idx);
                    /* Lower part + r2 -> r0, Note we can't use r0 directly */
                    fprintf( outfile, "    \"\\taddi %s, %s, %d\\n\"\n", ppc_reg(0), ppc_reg(2), j);
                    fprintf( outfile, "    \"\\tb %s\\n\"\n", asm_name("__wine_delay_load_asm") );
                    break;
                default:
                    /* On linux we can't use r2 since r2 is not a scratch register (hold the TOC) */
                    /* Save r13 on the stack */
                    fprintf( outfile, "    \"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
                    fprintf( outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg(13), ppc_reg(1));
                    /* Upper part in r13 */
                    fprintf( outfile, "    \"\\tlis %s, %d\\n\"\n", ppc_reg(13), idx);
                    /* Lower part + r13 -> r0, Note we can't use r0 directly */
                    fprintf( outfile, "    \"\\taddi %s, %s, %d\\n\"\n", ppc_reg(0), ppc_reg(13), j);
                    /* Restore r13 */
                    fprintf( outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg(13), ppc_reg(1));
                    fprintf( outfile, "    \"\\taddic %s, %s, 0x4\\n\"\n", ppc_reg(1), ppc_reg(1));
                    fprintf( outfile, "    \"\\tb %s\\n\"\n", asm_name("__wine_delay_load_asm") );
                    break;
                }
                break;
            }
            output_function_size( outfile, name );
        }
        idx++;
    }
    output_function_size( outfile, delayed_import_loaders );

    fprintf( outfile, "\n    \".align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name(delayed_import_thunks));
    pos = nb_delayed * 32;
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            output_import_thunk( outfile, odp->name ? odp->name : odp->export_name,
                                 "delay_imports", pos );
        }
    }
    output_function_size( outfile, delayed_import_thunks );
    fprintf( outfile, ");\n" );
}

/* output the import and delayed import tables of a Win32 module
 * returns number of DLLs exported in 'immediate' mode
 */
int output_imports( FILE *outfile, DLLSPEC *spec, int *nb_delayed )
{
    *nb_delayed = output_delayed_imports( outfile, spec );
    return output_immediate_imports( outfile );
}

/* output the import and delayed import thunks of a Win32 module */
void output_import_thunks( FILE *outfile, DLLSPEC *spec )
{
    output_delayed_import_thunks( outfile, spec );
    output_immediate_import_thunks( outfile );
}
