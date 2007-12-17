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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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

#include "windef.h"
#include "winbase.h"
#include "build.h"

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

struct name_table
{
    char **names;
    unsigned int count, size;
};

static struct name_table undef_symbols;    /* list of undefined symbols */
static struct name_table ignore_symbols;   /* list of symbols to ignore */
static struct name_table extra_ld_symbols; /* list of extra symbols that ld should resolve */
static struct name_table delayed_imports;  /* list of delayed import dlls */
static struct name_table ext_link_imports; /* list of external symbols to link to */

static struct import **dll_imports = NULL;
static int nb_imports = 0;      /* number of imported dlls (delayed or not) */
static int nb_delayed = 0;      /* number of delayed dlls */
static int total_imports = 0;   /* total number of imported functions */
static int total_delayed = 0;   /* total number of imported functions in delayed DLLs */


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

/* add a name to a name table */
static inline void add_name( struct name_table *table, const char *name )
{
    if (table->count == table->size)
    {
        table->size += (table->size / 2);
        if (table->size < 32) table->size = 32;
        table->names = xrealloc( table->names, table->size * sizeof(*table->names) );
    }
    table->names[table->count++] = xstrdup( name );
}

/* remove a name from a name table */
static inline void remove_name( struct name_table *table, unsigned int idx )
{
    assert( idx < table->count );
    free( table->names[idx] );
    memmove( table->names + idx, table->names + idx + 1,
             (table->count - idx - 1) * sizeof(*table->names) );
    table->count--;
}

/* make a name table empty */
static inline void empty_name_table( struct name_table *table )
{
    unsigned int i;

    for (i = 0; i < table->count; i++) free( table->names[i] );
    table->count = 0;
}

/* locate a name in a (sorted) list */
static inline const char *find_name( const char *name, const struct name_table *table )
{
    char **res = NULL;

    if (table->count) res = bsearch( &name, table->names, table->count, sizeof(*table->names), name_cmp );
    return res ? *res : NULL;
}

/* sort a name table */
static inline void sort_names( struct name_table *table )
{
    if (table->count) qsort( table->names, table->count, sizeof(*table->names), name_cmp );
}

/* locate an export in a (sorted) export list */
static inline ORDDEF *find_export( const char *name, ORDDEF **table, int size )
{
    ORDDEF func, *odp, **res = NULL;

    func.name = xstrdup(name);
    func.ordinal = -1;
    odp = &func;
    if (table) res = bsearch( &odp, table, size, sizeof(*table), func_cmp );
    free( func.name );
    return res ? *res : NULL;
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

/* check whether a given dll is imported in delayed mode */
static int is_delayed_import( const char *name )
{
    unsigned int i;

    for (i = 0; i < delayed_imports.count; i++)
    {
        if (!strcmp( delayed_imports.names[i], name )) return 1;
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

    if (spec->nb_entry_points)
    {
        imp->exports = xmalloc( spec->nb_entry_points * sizeof(*imp->exports) );
        for (i = 0; i < spec->nb_entry_points; i++)
            imp->exports[imp->nb_exports++] = &spec->entry_points[i];
        qsort( imp->exports, imp->nb_exports, sizeof(*imp->exports), func_cmp );
    }
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

    add_name( &delayed_imports, fullname );
    if ((imp = is_already_imported( fullname )) && !imp->delay)
    {
        imp->delay = 1;
        nb_delayed++;
    }
    free( fullname );
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

/* add a symbol to the ignored symbol list */
/* if the name starts with '-' the symbol is removed instead */
void add_ignore_symbol( const char *name )
{
    unsigned int i;

    if (name[0] == '-')  /* remove it */
    {
        if (!name[1]) empty_name_table( &ignore_symbols );  /* remove everything */
        else for (i = 0; i < ignore_symbols.count; i++)
        {
            if (!strcmp( ignore_symbols.names[i], name+1 )) remove_name( &ignore_symbols, i-- );
        }
    }
    else add_name( &ignore_symbols, name );
}

/* add a symbol to the list of extra symbols that ld must resolve */
void add_extra_ld_symbol( const char *name )
{
    add_name( &extra_ld_symbols, name );
}

/* add a function to the list of imports from a given dll */
static void add_import_func( struct import *imp, ORDDEF *func )
{
    imp->imports = xrealloc( imp->imports, (imp->nb_imports+1) * sizeof(*imp->imports) );
    imp->imports[imp->nb_imports++] = func;
    total_imports++;
    if (imp->delay) total_delayed++;
}

/* get the default entry point for a given spec file */
static const char *get_default_entry_point( const DLLSPEC *spec )
{
    if (spec->characteristics & IMAGE_FILE_DLL) return "__wine_spec_dll_entry";
    if (spec->subsystem == IMAGE_SUBSYSTEM_NATIVE) return "__wine_spec_drv_entry";
    return "__wine_spec_exe_entry";
}

/* check if the spec file exports any stubs */
static int has_stubs( const DLLSPEC *spec )
{
    int i;
    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB) return 1;
    }
    return 0;
}

/* add the extra undefined symbols that will be contained in the generated spec file itself */
static void add_extra_undef_symbols( DLLSPEC *spec )
{
    if (!spec->init_func) spec->init_func = xstrdup( get_default_entry_point(spec) );
    add_extra_ld_symbol( spec->init_func );
    if (has_stubs( spec )) add_extra_ld_symbol( "__wine_spec_unimplemented_stub" );
    if (nb_delayed) add_extra_ld_symbol( "__wine_spec_delay_load" );
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

/* check if a given forward does exist in one of the imported dlls */
static void check_undefined_forwards( DLLSPEC *spec )
{
    char *link_name, *api_name, *dll_name, *p;
    int i, j;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];

        if (!(odp->flags & FLAG_FORWARD)) continue;

        link_name = xstrdup( odp->link_name );
        p = strrchr( link_name, '.' );
        *p = 0;
        api_name = p + 1;
        dll_name = get_dll_name( link_name, NULL );

        for (j = 0; j < nb_imports; j++)
        {
            struct import *imp = dll_imports[j];

            if (strcasecmp( imp->spec->file_name, dll_name )) continue;
            if (!find_export( api_name, imp->exports, imp->nb_exports ))
                warning( "%s:%d: forward '%s' not found in %s\n",
                         spec->src_name, odp->lineno, odp->link_name, imp->spec->file_name );
            break;
        }
        if (j == nb_imports)
            warning( "%s:%d: forward '%s' not found in the imported dll list\n",
                     spec->src_name, odp->lineno, odp->link_name );
        free( link_name );
        free( dll_name );
    }
}

/* flag the dll exports that link to an undefined symbol */
static void check_undefined_exports( DLLSPEC *spec )
{
    int i;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        if (find_name( odp->link_name, &undef_symbols ))
        {
            switch(odp->type)
            {
            case TYPE_PASCAL:
            case TYPE_STDCALL:
            case TYPE_CDECL:
            case TYPE_VARARGS:
                if (link_ext_symbols)
                {
                    odp->flags |= FLAG_EXT_LINK;
                    add_name( &ext_link_imports, odp->link_name );
                }
                else error( "%s:%d: function '%s' not defined\n",
                            spec->src_name, odp->lineno, odp->link_name );
                break;
            default:
                error( "%s:%d: external symbol '%s' is not a function\n",
                       spec->src_name, odp->lineno, odp->link_name );
                break;
            }
        }
    }
}

/* create a .o file that references all the undefined symbols we want to resolve */
static char *create_undef_symbols_file( DLLSPEC *spec )
{
    char *as_file, *obj_file;
    int i;
    unsigned int j;
    FILE *f;

    as_file = get_temp_file_name( output_file_name, ".s" );
    if (!(f = fopen( as_file, "w" ))) fatal_error( "Cannot create %s\n", as_file );
    fprintf( f, "\t.data\n" );

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        fprintf( f, "\t%s %s\n", get_asm_ptr_keyword(), asm_name(odp->link_name) );
    }
    for (j = 0; j < extra_ld_symbols.count; j++)
        fprintf( f, "\t%s %s\n", get_asm_ptr_keyword(), asm_name(extra_ld_symbols.names[j]) );
    fclose( f );

    obj_file = get_temp_file_name( output_file_name, ".o" );
    assemble_file( as_file, obj_file );
    return obj_file;
}

/* combine a list of object files with ld into a single object file */
/* returns the name of the combined file */
static const char *ldcombine_files( DLLSPEC *spec, char **argv )
{
    unsigned int i, len = 0;
    char *cmd, *p, *ld_tmp_file, *undef_file;
    int err;

    undef_file = create_undef_symbols_file( spec );
    len += strlen(undef_file) + 1;
    ld_tmp_file = get_temp_file_name( output_file_name, ".o" );
    if (!ld_command) ld_command = xstrdup("ld");
    for (i = 0; argv[i]; i++) len += strlen(argv[i]) + 1;
    cmd = p = xmalloc( len + strlen(ld_tmp_file) + 8 + strlen(ld_command)  );
    p += sprintf( cmd, "%s -r -o %s %s", ld_command, ld_tmp_file, undef_file );
    for (i = 0; argv[i]; i++)
        p += sprintf( p, " %s", argv[i] );
    if (verbose) fprintf( stderr, "%s\n", cmd );
    err = system( cmd );
    if (err) fatal_error( "%s -r failed with status %d\n", ld_command, err );
    free( cmd );
    return ld_tmp_file;
}

/* read in the list of undefined symbols */
void read_undef_symbols( DLLSPEC *spec, char **argv )
{
    size_t prefix_len;
    FILE *f;
    char *cmd, buffer[1024], name_prefix[16];
    int err;
    const char *name;

    if (!argv[0]) return;

    add_extra_undef_symbols( spec );

    strcpy( name_prefix, asm_name("") );
    prefix_len = strlen( name_prefix );

    name = ldcombine_files( spec, argv );

    if (!nm_command) nm_command = xstrdup("nm");
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
        add_name( &undef_symbols, p );
    }
    if ((err = pclose( f ))) warning( "%s failed with status %d\n", cmd, err );
    free( cmd );
}

/* resolve the imports for a Win32 module */
int resolve_imports( DLLSPEC *spec )
{
    int i;
    unsigned int j, removed;
    ORDDEF *odp;

    sort_names( &ignore_symbols );
    check_undefined_forwards( spec );

    for (i = 0; i < nb_imports; i++)
    {
        struct import *imp = dll_imports[i];

        for (j = removed = 0; j < undef_symbols.count; j++)
        {
            if (find_name( undef_symbols.names[j], &ignore_symbols )) continue;
            odp = find_export( undef_symbols.names[j], imp->exports, imp->nb_exports );
            if (odp)
            {
                if (odp->flags & FLAG_PRIVATE) continue;
                if (odp->type != TYPE_STDCALL && odp->type != TYPE_CDECL)
                    warning( "winebuild: Data export '%s' cannot be imported from %s\n",
                             odp->link_name, imp->spec->file_name );
                else
                {
                    add_import_func( imp, odp );
                    remove_name( &undef_symbols, j-- );
                    removed++;
                }
            }
        }
        if (!removed)
        {
            /* the dll is not used, get rid of it */
            if (check_unused( imp, spec ))
                warning( "winebuild: %s imported but no symbols used\n", imp->spec->file_name );
            remove_import_dll( i );
            i--;
        }
    }

    sort_names( &undef_symbols );
    check_undefined_exports( spec );

    return 1;
}

/* output the get_pc thunk if needed */
void output_get_pc_thunk(void)
{
    if (target_cpu != CPU_x86) return;
    if (!UsePIC) return;
    output( "\n\t.text\n" );
    output( "\t.align %d\n", get_alignment(4) );
    output( "\t%s\n", func_declaration("__wine_spec_get_pc_thunk_eax") );
    output( "%s:\n", asm_name("__wine_spec_get_pc_thunk_eax") );
    output( "\tpopl %%eax\n" );
    output( "\tpushl %%eax\n" );
    output( "\tret\n" );
    output_function_size( "__wine_spec_get_pc_thunk_eax" );
}

/* output a single import thunk */
static void output_import_thunk( const char *name, const char *table, int pos )
{
    output( "\n\t.align %d\n", get_alignment(4) );
    output( "\t%s\n", func_declaration(name) );
    output( "%s\n", asm_globl(name) );

    switch(target_cpu)
    {
    case CPU_x86:
        if (!UsePIC)
        {
            output( "\tjmp *(%s+%d)\n", table, pos );
        }
        else
        {
            output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
            output( "1:\tjmp *%s+%d-1b(%%eax)\n", table, pos );
        }
        break;
    case CPU_x86_64:
        output( "\tjmpq *%s+%d(%%rip)\n", table, pos );
        break;
    case CPU_SPARC:
        if ( !UsePIC )
        {
            output( "\tsethi %%hi(%s+%d), %%g1\n", table, pos );
            output( "\tld [%%g1+%%lo(%s+%d)], %%g1\n", table, pos );
            output( "\tjmp %%g1\n" );
            output( "\tnop\n" );
        }
        else
        {
            /* Hmpf.  Stupid sparc assembler always interprets global variable
               names as GOT offsets, so we have to do it the long way ... */
            output( "\tsave %%sp, -96, %%sp\n" );
            output( "0:\tcall 1f\n" );
            output( "\tnop\n" );
            output( "1:\tsethi %%hi(%s+%d-0b), %%g1\n", table, pos );
            output( "\tor %%g1, %%lo(%s+%d-0b), %%g1\n", table, pos );
            output( "\tld [%%g1+%%o7], %%g1\n" );
            output( "\tjmp %%g1\n" );
            output( "\trestore\n" );
        }
        break;
    case CPU_ALPHA:
        output( "\tlda $0,%s\n", table );
        output( "\tlda $0,%d($0)\n", pos );
        output( "\tjmp $31,($0)\n" );
        break;
    case CPU_POWERPC:
        output( "\taddi %s, %s, -0x4\n", ppc_reg(1), ppc_reg(1) );
        output( "\tstw  %s, 0(%s)\n",    ppc_reg(9), ppc_reg(1) );
        output( "\taddi %s, %s, -0x4\n", ppc_reg(1), ppc_reg(1) );
        output( "\tstw  %s, 0(%s)\n",    ppc_reg(8), ppc_reg(1) );
        output( "\taddi %s, %s, -0x4\n", ppc_reg(1), ppc_reg(1) );
        output( "\tstw  %s, 0(%s)\n",    ppc_reg(7), ppc_reg(1) );
        if (target_platform == PLATFORM_APPLE)
        {
            output( "\tlis %s, ha16(%s+%d)\n", ppc_reg(9), table, pos );
            output( "\tla  %s, lo16(%s+%d)(%s)\n", ppc_reg(8), table, pos, ppc_reg(9) );
        }
        else
        {
            output( "\tlis %s, (%s+%d)@h\n", ppc_reg(9), table, pos );
            output( "\tla  %s, (%s+%d)@l(%s)\n", ppc_reg(8), table, pos, ppc_reg(9) );
        }
        output( "\tlwz  %s, 0(%s)\n", ppc_reg(7), ppc_reg(8) );
        output( "\tmtctr %s\n", ppc_reg(7) );
        output( "\tlwz  %s, 0(%s)\n",   ppc_reg(7), ppc_reg(1) );
        output( "\taddi %s, %s, 0x4\n", ppc_reg(1), ppc_reg(1) );
        output( "\tlwz  %s, 0(%s)\n",   ppc_reg(8), ppc_reg(1) );
        output( "\taddi %s, %s, 0x4\n", ppc_reg(1), ppc_reg(1) );
        output( "\tlwz  %s, 0(%s)\n",   ppc_reg(9), ppc_reg(1) );
        output( "\taddi %s, %s, 0x4\n", ppc_reg(1), ppc_reg(1) );
        output( "\tbctr\n" );
        break;
    }
    output_function_size( name );
}

/* check if we need an import directory */
int has_imports(void)
{
    return (nb_imports - nb_delayed) > 0;
}

/* output the import table of a Win32 module */
static void output_immediate_imports(void)
{
    int i, j;
    const char *dll_name;

    if (nb_imports == nb_delayed) return;  /* no immediate imports */

    /* main import header */

    output( "\n/* import table */\n" );
    output( "\n\t.data\n" );
    output( "\t.align %d\n", get_alignment(4) );
    output( ".L__wine_spec_imports:\n" );

    /* list of dlls */

    for (i = j = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        dll_name = make_c_identifier( dll_imports[i]->spec->file_name );
        output( "\t.long 0\n" );     /* OriginalFirstThunk */
        output( "\t.long 0\n" );     /* TimeDateStamp */
        output( "\t.long 0\n" );     /* ForwarderChain */
        output( "\t.long .L__wine_spec_import_name_%s-.L__wine_spec_rva_base\n", /* Name */
                 dll_name );
        output( "\t.long .L__wine_spec_import_data_ptrs+%d-.L__wine_spec_rva_base\n",  /* FirstThunk */
                 j * get_ptr_size() );
        j += dll_imports[i]->nb_imports + 1;
    }
    output( "\t.long 0\n" );     /* OriginalFirstThunk */
    output( "\t.long 0\n" );     /* TimeDateStamp */
    output( "\t.long 0\n" );     /* ForwarderChain */
    output( "\t.long 0\n" );     /* Name */
    output( "\t.long 0\n" );     /* FirstThunk */

    output( "\n\t.align %d\n", get_alignment(get_ptr_size()) );
    output( ".L__wine_spec_import_data_ptrs:\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        dll_name = make_c_identifier( dll_imports[i]->spec->file_name );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            if (!(odp->flags & FLAG_NONAME))
                output( "\t%s .L__wine_spec_import_data_%s_%s-.L__wine_spec_rva_base\n",
                         get_asm_ptr_keyword(), dll_name, odp->name );
            else
            {
                if (get_ptr_size() == 8)
                    output( "\t.quad 0x800000000000%04x\n", odp->ordinal );
                else
                    output( "\t.long 0x8000%04x\n", odp->ordinal );
            }
        }
        output( "\t%s 0\n", get_asm_ptr_keyword() );
    }
    output( ".L__wine_spec_imports_end:\n" );

    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        dll_name = make_c_identifier( dll_imports[i]->spec->file_name );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            if (!(odp->flags & FLAG_NONAME))
            {
                output( "\t.align %d\n", get_alignment(2) );
                output( ".L__wine_spec_import_data_%s_%s:\n", dll_name, odp->name );
                output( "\t%s %d\n", get_asm_short_keyword(), odp->ordinal );
                output( "\t%s \"%s\"\n", get_asm_string_keyword(), odp->name );
            }
        }
    }

    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        dll_name = make_c_identifier( dll_imports[i]->spec->file_name );
        output( ".L__wine_spec_import_name_%s:\n\t%s \"%s\"\n",
                 dll_name, get_asm_string_keyword(), dll_imports[i]->spec->file_name );
    }
}

/* output the import thunks of a Win32 module */
static void output_immediate_import_thunks(void)
{
    int i, j, pos;
    int nb_imm = nb_imports - nb_delayed;
    static const char import_thunks[] = "__wine_spec_import_thunks";

    if (!nb_imm) return;

    output( "\n/* immediate import thunks */\n\n" );
    output( "\t.text\n" );
    output( "\t.align %d\n", get_alignment(8) );
    output( "%s:\n", asm_name(import_thunks));

    for (i = pos = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += get_ptr_size())
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            output_import_thunk( odp->name ? odp->name : odp->export_name,
                                 ".L__wine_spec_import_data_ptrs", pos );
        }
        pos += get_ptr_size();
    }
    output_function_size( import_thunks );
}

/* output the delayed import table of a Win32 module */
static void output_delayed_imports( const DLLSPEC *spec )
{
    int i, j, mod;

    if (!nb_delayed) return;

    output( "\n/* delayed imports */\n\n" );
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( "%s\n", asm_globl("__wine_spec_delay_imports") );

    /* list of dlls */

    for (i = j = mod = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* grAttrs */
        output( "\t%s .L__wine_delay_name_%d\n",       /* szName */
                 get_asm_ptr_keyword(), i );
        output( "\t%s .L__wine_delay_modules+%d\n",    /* phmod */
                 get_asm_ptr_keyword(), mod * get_ptr_size() );
        output( "\t%s .L__wine_delay_IAT+%d\n",        /* pIAT */
                 get_asm_ptr_keyword(), j * get_ptr_size() );
        output( "\t%s .L__wine_delay_INT+%d\n",        /* pINT */
                 get_asm_ptr_keyword(), j * get_ptr_size() );
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pBoundIAT */
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pUnloadIAT */
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* dwTimeStamp */
        j += dll_imports[i]->nb_imports;
        mod++;
    }
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* grAttrs */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* szName */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* phmod */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pIAT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pINT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pBoundIAT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pUnloadIAT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* dwTimeStamp */

    output( "\n.L__wine_delay_IAT:\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;
            output( "\t%s .L__wine_delay_imp_%d_%s\n",
                     get_asm_ptr_keyword(), i, name );
        }
    }

    output( "\n.L__wine_delay_INT:\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            if (!odp->name)
                output( "\t%s %d\n", get_asm_ptr_keyword(), odp->ordinal );
            else
                output( "\t%s .L__wine_delay_data_%d_%s\n",
                         get_asm_ptr_keyword(), i, odp->name );
        }
    }

    output( "\n.L__wine_delay_modules:\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) output( "\t%s 0\n", get_asm_ptr_keyword() );
    }

    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        output( ".L__wine_delay_name_%d:\n", i );
        output( "\t%s \"%s\"\n",
                 get_asm_string_keyword(), dll_imports[i]->spec->file_name );
    }

    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            if (!odp->name) continue;
            output( ".L__wine_delay_data_%d_%s:\n", i, odp->name );
            output( "\t%s \"%s\"\n", get_asm_string_keyword(), odp->name );
        }
    }
    output_function_size( "__wine_spec_delay_imports" );
}

/* output the delayed import thunks of a Win32 module */
static void output_delayed_import_thunks( const DLLSPEC *spec )
{
    int i, idx, j, pos, extra_stack_storage = 0;
    static const char delayed_import_loaders[] = "__wine_spec_delayed_import_loaders";
    static const char delayed_import_thunks[] = "__wine_spec_delayed_import_thunks";

    if (!nb_delayed) return;

    output( "\n/* delayed import thunks */\n\n" );
    output( "\t.text\n" );
    output( "\t.align %d\n", get_alignment(8) );
    output( "%s:\n", asm_name(delayed_import_loaders));
    output( "\t%s\n", func_declaration("__wine_delay_load_asm") );
    output( "%s:\n", asm_name("__wine_delay_load_asm") );
    switch(target_cpu)
    {
    case CPU_x86:
        output( "\tpushl %%ecx\n" );
        output( "\tpushl %%edx\n" );
        output( "\tpushl %%eax\n" );
        output( "\tcall %s\n", asm_name("__wine_spec_delay_load") );
        output( "\tpopl %%edx\n" );
        output( "\tpopl %%ecx\n" );
        output( "\tjmp *%%eax\n" );
        break;
    case CPU_x86_64:
        output( "\tpushq %%rdi\n" );
        output( "\tpushq %%rsi\n" );
        output( "\tpushq %%rdx\n" );
        output( "\tpushq %%rcx\n" );
        output( "\tpushq %%r8\n" );
        output( "\tpushq %%r9\n" );
        output( "\tsubq $8,%%rsp\n" );
        output( "\tmovq %%r11,%%rdi\n" );
        output( "\tcall %s\n", asm_name("__wine_spec_delay_load") );
        output( "\taddq $8,%%rsp\n" );
        output( "\tpopq %%r9\n" );
        output( "\tpopq %%r8\n" );
        output( "\tpopq %%rcx\n" );
        output( "\tpopq %%rdx\n" );
        output( "\tpopq %%rsi\n" );
        output( "\tpopq %%rdi\n" );
        output( "\tjmp *%%rax\n" );
        break;
    case CPU_SPARC:
        output( "\tsave %%sp, -96, %%sp\n" );
        output( "\tcall %s\n", asm_name("__wine_spec_delay_load") );
        output( "\tmov %%g1, %%o0\n" );
        output( "\tjmp %%o0\n" );
        output( "\trestore\n" );
        break;
    case CPU_ALPHA:
        output( "\tjsr $26,%s\n", asm_name("__wine_spec_delay_load") );
        output( "\tjmp $31,($0)\n" );
        break;
    case CPU_POWERPC:
        if (target_platform == PLATFORM_APPLE) extra_stack_storage = 56;

        /* Save all callee saved registers into a stackframe. */
        output( "\tstwu %s, -%d(%s)\n",ppc_reg(1), 48+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(3),  4+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(4),  8+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(5), 12+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(6), 16+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(7), 20+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(8), 24+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(9), 28+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(10),32+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(11),36+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(12),40+extra_stack_storage, ppc_reg(1));

        /* r0 -> r3 (arg1) */
        output( "\tmr %s, %s\n", ppc_reg(3), ppc_reg(0));

        /* save return address */
        output( "\tmflr %s\n", ppc_reg(0));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(0), 44+extra_stack_storage, ppc_reg(1));

        /* Call the __wine_delay_load function, arg1 is arg1. */
        output( "\tbl %s\n", asm_name("__wine_spec_delay_load") );

        /* Load return value from call into ctr register */
        output( "\tmtctr %s\n", ppc_reg(3));

        /* restore all saved registers and drop stackframe. */
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(3),  4+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(4),  8+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(5), 12+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(6), 16+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(7), 20+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(8), 24+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(9), 28+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(10),32+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(11),36+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(12),40+extra_stack_storage, ppc_reg(1));

        /* Load return value from call into return register */
        output( "\tlwz  %s,  %d(%s)\n", ppc_reg(0), 44+extra_stack_storage, ppc_reg(1));
        output( "\tmtlr %s\n", ppc_reg(0));
        output( "\taddi %s, %s, %d\n", ppc_reg(1), ppc_reg(1),  48+extra_stack_storage);

        /* branch to ctr register. */
        output( "\tbctr\n");
        break;
    }
    output_function_size( "__wine_delay_load_asm" );
    output( "\n" );

    for (i = idx = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;

            output( ".L__wine_delay_imp_%d_%s:\n", i, name );
            switch(target_cpu)
            {
            case CPU_x86:
                output( "\tmovl $%d, %%eax\n", (idx << 16) | j );
                output( "\tjmp %s\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_x86_64:
                output( "\tmovq $%d,%%r11\n", (idx << 16) | j );
                output( "\tjmp %s\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_SPARC:
                output( "\tset %d, %%g1\n", (idx << 16) | j );
                output( "\tb,a %s\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_ALPHA:
                output( "\tlda $0,%d($31)\n", j);
                output( "\tldah $0,%d($0)\n", idx);
                output( "\tjmp $31,%s\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_POWERPC:
                switch(target_platform)
                {
                case PLATFORM_APPLE:
                    /* On Darwin we can use r0 and r2 */
                    /* Upper part in r2 */
                    output( "\tlis %s, %d\n", ppc_reg(2), idx);
                    /* Lower part + r2 -> r0, Note we can't use r0 directly */
                    output( "\taddi %s, %s, %d\n", ppc_reg(0), ppc_reg(2), j);
                    output( "\tb %s\n", asm_name("__wine_delay_load_asm") );
                    break;
                default:
                    /* On linux we can't use r2 since r2 is not a scratch register (hold the TOC) */
                    /* Save r13 on the stack */
                    output( "\taddi %s, %s, -0x4\n", ppc_reg(1), ppc_reg(1));
                    output( "\tstw  %s, 0(%s)\n",    ppc_reg(13), ppc_reg(1));
                    /* Upper part in r13 */
                    output( "\tlis %s, %d\n", ppc_reg(13), idx);
                    /* Lower part + r13 -> r0, Note we can't use r0 directly */
                    output( "\taddi %s, %s, %d\n", ppc_reg(0), ppc_reg(13), j);
                    /* Restore r13 */
                    output( "\tstw  %s, 0(%s)\n",    ppc_reg(13), ppc_reg(1));
                    output( "\taddic %s, %s, 0x4\n", ppc_reg(1), ppc_reg(1));
                    output( "\tb %s\n", asm_name("__wine_delay_load_asm") );
                    break;
                }
                break;
            }
        }
        idx++;
    }
    output_function_size( delayed_import_loaders );

    output( "\n\t.align %d\n", get_alignment(get_ptr_size()) );
    output( "%s:\n", asm_name(delayed_import_thunks));
    for (i = pos = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += get_ptr_size())
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            output_import_thunk( odp->name ? odp->name : odp->export_name,
                                 ".L__wine_delay_IAT", pos );
        }
    }
    output_function_size( delayed_import_thunks );
}

/* output import stubs for exported entry points that link to external symbols */
static void output_external_link_imports( DLLSPEC *spec )
{
    unsigned int i, pos;

    if (!ext_link_imports.count) return;  /* nothing to do */

    sort_names( &ext_link_imports );

    /* get rid of duplicate names */
    for (i = 1; i < ext_link_imports.count; i++)
    {
        if (!strcmp( ext_link_imports.names[i-1], ext_link_imports.names[i] ))
            remove_name( &ext_link_imports, i-- );
    }

    output( "\n/* external link thunks */\n\n" );
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( ".L__wine_spec_external_links:\n" );
    for (i = 0; i < ext_link_imports.count; i++)
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name(ext_link_imports.names[i]) );

    output( "\n\t.text\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( "%s:\n", asm_name("__wine_spec_external_link_thunks") );

    for (i = pos = 0; i < ext_link_imports.count; i++)
    {
        char buffer[256];
        sprintf( buffer, "__wine_spec_ext_link_%s", ext_link_imports.names[i] );
        output_import_thunk( buffer, ".L__wine_spec_external_links", pos );
        pos += get_ptr_size();
    }
    output_function_size( "__wine_spec_external_link_thunks" );
}

/*******************************************************************
 *         output_stubs
 *
 * Output the functions for stub entry points
 */
void output_stubs( DLLSPEC *spec )
{
    const char *name, *exp_name;
    int i, count;

    if (!has_stubs( spec )) return;

    output( "\n/* stub functions */\n\n" );
    output( "\t.text\n" );

    for (i = count = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;

        name = get_stub_name( odp, spec );
        exp_name = odp->name ? odp->name : odp->export_name;
        output( "\t.align %d\n", get_alignment(4) );
        output( "\t%s\n", func_declaration(name) );
        output( "%s:\n", asm_name(name) );

        /* flesh out the stub a bit to make safedisc happy */
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );
        output(" \tnop\n" );

        output( "\tsubl $4,%%esp\n" );
        if (UsePIC)
        {
            output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
            output( "1:" );
            if (exp_name)
            {
                output( "\tleal .L%s_string-1b(%%eax),%%ecx\n", name );
                output( "\tpushl %%ecx\n" );
                count++;
            }
            else
                output( "\tpushl $%d\n", odp->ordinal );
            output( "\tleal .L__wine_spec_file_name-1b(%%eax),%%ecx\n" );
            output( "\tpushl %%ecx\n" );
        }
        else
        {
            if (exp_name)
            {
                output( "\tpushl $.L%s_string\n", name );
                count++;
            }
            else
                output( "\tpushl $%d\n", odp->ordinal );
            output( "\tpushl $.L__wine_spec_file_name\n" );
        }
        output( "\tcall %s\n", asm_name("__wine_spec_unimplemented_stub") );
        output_function_size( name );
    }

    if (count)
    {
        output( "\t%s\n", get_asm_string_section() );
        for (i = 0; i < spec->nb_entry_points; i++)
        {
            ORDDEF *odp = &spec->entry_points[i];
            if (odp->type != TYPE_STUB) continue;
            exp_name = odp->name ? odp->name : odp->export_name;
            if (exp_name)
            {
                name = get_stub_name( odp, spec );
                output( ".L%s_string:\n", name );
                output( "\t%s \"%s\"\n", get_asm_string_keyword(), exp_name );
            }
        }
    }
}

/* output the import and delayed import tables of a Win32 module */
void output_imports( DLLSPEC *spec )
{
    output_immediate_imports();
    output_delayed_imports( spec );
    output_immediate_import_thunks();
    output_delayed_import_thunks( spec );
    output_external_link_imports( spec );
    if (nb_imports || ext_link_imports.count || has_stubs(spec) || has_relays(spec))
        output_get_pc_thunk();
}
