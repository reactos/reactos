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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winglue.h"
#include "build.h"

struct import
{
    DLLSPEC     *spec;         /* description of the imported dll */
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

#ifdef __powerpc__
# ifdef __APPLE__
# define ppc_high(mem) "ha16(" mem ")"
# define ppc_low(mem)  "lo16(" mem ")"
static const char * const ppc_reg[32] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                          "r8", "r9", "r10","r11","r12","r13","r14","r15",
                                          "r16","r17","r18","r19","r20","r21","r22","r23",
                                          "r24","r25","r26","r27","r28","r29","r30","r31" };
# else  /* __APPLE__ */
# define ppc_high(mem) "(" mem ")@hi"
# define ppc_low(mem)  "(" mem ")@l"
static const char * const ppc_reg[32] = { "0", "1", "2", "3", "4", "5", "6", "7",
                                          "8", "9", "10","11","12","13","14","15",
                                          "16","17","18","19","20","21","22","23",
                                          "24","25","26","27","28","29","30","31" };
# endif  /* __APPLE__ */
#endif  /* __powerpc__ */

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
#ifdef HAVE_ASM_DOT_SIZE
    fprintf( outfile, "    \"\\t.size " __ASM_NAME("%s") ", . - " __ASM_NAME("%s") "\\n\"\n", name, name);
#endif
}

/* free an import structure */
static void free_imports( struct import *imp )
{
    free( imp->exports );
    free( imp->imports );
    free_dll_spec( imp->spec );
    free( imp );
}

/* remove the temp file at exit */
static void remove_ld_tmp_file(void)
{
    if (ld_tmp_file) unlink( ld_tmp_file );
}

/* check whether a given dll has already been imported */
static int is_already_imported( const char *name )
{
    int i;

    for (i = 0; i < nb_imports; i++)
    {
        if (!strcmp( dll_imports[i]->spec->file_name, name )) return 1;
    }
    return 0;
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

/* open the .so library for a given dll */
static char *open_library( const char *name )
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
static int read_import_lib( const char *name, struct import *imp )
{
    FILE *f;
    char *fullname;
    int i, ret;
    DLLSPEC *spec = imp->spec;

    imp->exports    = NULL;
    imp->nb_exports = 0;

    fullname = open_library( name );
    f = open_input_file( NULL, fullname );
    free( fullname );

    ret = parse_def_file( f, spec );
    close_input_file( f );
    if (!ret) return 0;
    if (is_already_imported( spec->file_name )) return 0;

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

/* add a dll to the list of imports */
void add_import_dll( const char *name, int delay )
{
    struct import *imp;
    char *fullname;

    fullname = xmalloc( strlen(name) + 5 );
    strcpy( fullname, name );
    if (!strchr( fullname, '.' )) strcat( fullname, ".dll" );

    /* check if we already imported it */
    if (is_already_imported( fullname ))
    {
        free( fullname );
        return;
    }

    imp = xmalloc( sizeof(*imp) );
    imp->spec            = alloc_dll_spec();
    imp->spec->file_name = fullname;
    imp->delay           = delay;
    imp->imports         = NULL;
    imp->nb_imports      = 0;
    if (delay) nb_delayed++;

    if (read_import_lib( name, imp ))
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
    int i, count = 0, nb_stubs = 0, nb_regs = 0;
    int kernel_imports = 0, ntdll_imports = 0;

    sort_symbols( undef_symbols, nb_undef_symbols );

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB) nb_stubs++;
        if (odp->flags & FLAG_REGISTER) nb_regs++;
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
        kernel_imports += add_extra_symbol( extras, &count, "GetProcAddress", spec );
    }
    if (nb_regs)
        ntdll_imports += add_extra_symbol( extras, &count, "__wine_call_from_32_regs", spec );
    if (nb_delayed || nb_stubs)
        ntdll_imports += add_extra_symbol( extras, &count, "RtlRaiseException", spec );

    /* make sure we import the dlls that contain these functions */
    if (kernel_imports) add_import_dll( "kernel32", 0 );
    if (ntdll_imports) add_import_dll( "ntdll", 0 );

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
    char *cmd, *ldcmd;
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

    ldcmd = getenv("LD");
    if (!ldcmd) ldcmd = "ld";
    for (i = 0; argv[i]; i++) len += strlen(argv[i]) + 1;
    cmd = xmalloc( len + strlen(ld_tmp_file) + 8 + strlen(ldcmd)  );
    sprintf( cmd, "%s -r -o %s", ldcmd, ld_tmp_file );
    for (i = 0; argv[i]; i++) sprintf( cmd + strlen(cmd), " %s", argv[i] );
    err = system( cmd );
    if (err) fatal_error( "ld -r failed with status %d\n", err );
    free( cmd );
    return ld_tmp_file;
}

/* read in the list of undefined symbols */
void read_undef_symbols( char **argv )
{
    static const char name_prefix[] = __ASM_NAME("");
    static const int prefix_len = sizeof(name_prefix) - 1;
    FILE *f;
    char buffer[1024];
    int err;
    const char *name;

    if (!argv[0]) return;

    undef_size = nb_undef_symbols = 0;

    /* if we have multiple object files, link them together */
    if (argv[1]) name = ldcombine_files( argv );
    else name = argv[0];

    sprintf( buffer, "nm -u %s", name );
    if (!(f = popen( buffer, "r" )))
        fatal_error( "Cannot execute '%s'\n", buffer );

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
    if ((err = pclose( f ))) warning( "nm -u %s error %d\n", name, err );
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

/* output the import table of a Win32 module */
static int output_immediate_imports( FILE *outfile )
{
    int i, j, pos;
    int nb_imm = nb_imports - nb_delayed;
    static const char import_thunks[] = "__wine_spec_import_thunks";

    if (!nb_imm) goto done;

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

    /* thunks for imported functions */

    fprintf( outfile, "#ifndef __GNUC__\nstatic void __asm__dummy_import(void) {\n#endif\n\n" );
    pos = 20 * (nb_imm + 1);  /* offset of imports.data from start of imports */
    fprintf( outfile, "asm(\".data\\n\\t.align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\"\n", import_thunks);

    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;
            fprintf( outfile, "    \"\\t" __ASM_FUNC("%s") "\\n\"\n", name );
            fprintf( outfile, "    \"\\t.globl " __ASM_NAME("%s") "\\n\"\n", name );
            fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\\t", name);

#if defined(__i386__)
            if (strstr( name, "__wine_call_from_16" ))
                fprintf( outfile, ".byte 0x2e\\n\\tjmp *(imports+%d)\\n\\tnop\\n", pos );
            else
                fprintf( outfile, "jmp *(imports+%d)\\n\\tmovl %%esi,%%esi\\n", pos );
#elif defined(__sparc__)
            if ( !UsePIC )
            {
                fprintf( outfile, "sethi %%hi(imports+%d), %%g1\\n\\t", pos );
                fprintf( outfile, "ld [%%g1+%%lo(imports+%d)], %%g1\\n\\t", pos );
                fprintf( outfile, "jmp %%g1\\n\\tnop\\n" );
            }
            else
            {
                /* Hmpf.  Stupid sparc assembler always interprets global variable
                   names as GOT offsets, so we have to do it the long way ... */
                fprintf( outfile, "save %%sp, -96, %%sp\\n" );
                fprintf( outfile, "0:\\tcall 1f\\n\\tnop\\n" );
                fprintf( outfile, "1:\\tsethi %%hi(imports+%d-0b), %%g1\\n\\t", pos );
                fprintf( outfile, "or %%g1, %%lo(imports+%d-0b), %%g1\\n\\t", pos );
                fprintf( outfile, "ld [%%g1+%%o7], %%g1\\n\\t" );
                fprintf( outfile, "jmp %%g1\\n\\trestore\\n" );
            }

#elif defined(__powerpc__)
            fprintf(outfile, "\taddi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf(outfile, "\t\"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[9], ppc_reg[1]);
            fprintf(outfile, "\t\"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf(outfile, "\t\"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[8], ppc_reg[1]);
            fprintf(outfile, "\t\"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf(outfile, "\t\"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[7], ppc_reg[1]);

            fprintf(outfile, "\t\"\\tlis %s, " ppc_high(__ASM_NAME("imports") "+ %d")  "\\n\"\n", ppc_reg[9], pos);
            fprintf(outfile, "\t\"\\tla  %s, " ppc_low (__ASM_NAME("imports") "+ %d") "(%s)\\n\"\n", ppc_reg[8], pos, ppc_reg[9]);
            fprintf(outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n", ppc_reg[7], ppc_reg[8]);
            fprintf(outfile, "\t\"\\tmtctr %s\\n\"\n", ppc_reg[7]);

            fprintf(outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg[7], ppc_reg[1]);
            fprintf(outfile, "\t\"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf(outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg[8], ppc_reg[1]);
            fprintf(outfile, "\t\"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf(outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg[9], ppc_reg[1]);
            fprintf(outfile, "\t\"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf(outfile, "\t\"\\tbctr\\n");
#else
#error You need to define import thunks for your architecture!
#endif
            fprintf( outfile, "\"\n" );
            output_function_size( outfile, name );
        }
        pos += 4;
    }
    output_function_size( outfile, import_thunks );
    fprintf( outfile, "    \".text\");\n#ifndef __GNUC__\n}\n#endif\n\n" );

 done:
    return nb_imm;
}

/* output the delayed import table of a Win32 module */
static int output_delayed_imports( FILE *outfile, const DLLSPEC *spec )
{
    int i, idx, j, pos;
    static const char delayed_import_loaders[] = "__wine_spec_delayed_import_loaders";
    static const char delayed_import_thunks[] = "__wine_spec_delayed_import_thunks";

    if (!nb_delayed) goto done;

    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "static void *__wine_delay_imp_%d_hmod;\n", i);
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
        fprintf( outfile, "    { 0, \"%s\", &__wine_delay_imp_%d_hmod, &delay_imports.IAT[%d], &delay_imports.INT[%d], 0, 0, 0 },\n",
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

    /* check if there's some stub defined. if so, exception struct
     *  is already defined, so don't emit it twice
     */
    for (i = 0; i < spec->nb_entry_points; i++) if (spec->entry_points[i].type == TYPE_STUB) break;

    if (i == spec->nb_entry_points) {
       fprintf( outfile, "struct exc_record {\n" );
       fprintf( outfile, "  unsigned int code, flags;\n" );
       fprintf( outfile, "  void *rec, *addr;\n" );
       fprintf( outfile, "  unsigned int params;\n" );
       fprintf( outfile, "  const void *info[15];\n" );
       fprintf( outfile, "};\n\n" );
       fprintf( outfile, "extern void __stdcall RtlRaiseException( struct exc_record * );\n" );
    }

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
    fprintf( outfile, "    struct exc_record rec;\n" );
    fprintf( outfile, "    rec.code    = 0x80000100;\n" );
    fprintf( outfile, "    rec.flags   = 1;\n" );
    fprintf( outfile, "    rec.rec     = 0;\n" );
    fprintf( outfile, "    rec.params  = 2;\n" );
    fprintf( outfile, "    rec.info[0] = imd->szName;\n" );
    fprintf( outfile, "    rec.info[1] = *pINT;\n" );
    fprintf( outfile, "#ifdef __GNUC__\n" );
    fprintf( outfile, "    rec.addr = __builtin_return_address(1);\n" );
    fprintf( outfile, "#else\n" );
    fprintf( outfile, "    rec.addr = 0;\n" );
    fprintf( outfile, "#endif\n" );
    fprintf( outfile, "    for (;;) RtlRaiseException( &rec );\n" );
    fprintf( outfile, "    return 0; /* shouldn't go here */\n" );
    fprintf( outfile, "  }\n}\n\n" );

    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy_delay_import(void) {\n" );
    fprintf( outfile, "#endif\n" );

    fprintf( outfile, "asm(\".align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\"\n", delayed_import_loaders);
    fprintf( outfile, "    \"\\t" __ASM_FUNC("__wine_delay_load_asm") "\\n\"\n" );
    fprintf( outfile, "    \"" __ASM_NAME("__wine_delay_load_asm") ":\\n\"\n" );
#if defined(__i386__)
    fprintf( outfile, "    \"\\tpushl %%ecx\\n\\tpushl %%edx\\n\\tpushl %%eax\\n\"\n" );
    fprintf( outfile, "    \"\\tcall __wine_delay_load\\n\"\n" );
    fprintf( outfile, "    \"\\tpopl %%edx\\n\\tpopl %%ecx\\n\\tjmp *%%eax\\n\"\n" );
#elif defined(__sparc__)
    fprintf( outfile, "    \"\\tsave %%sp, -96, %%sp\\n\"\n" );
    fprintf( outfile, "    \"\\tcall __wine_delay_load\\n\"\n" );
    fprintf( outfile, "    \"\\tmov %%g1, %%o0\\n\"\n" );
    fprintf( outfile, "    \"\\tjmp %%o0\\n\\trestore\\n\"\n" );
#elif defined(__powerpc__)
# if defined(__APPLE__)
/* On darwin an extra 56 bytes must be allowed for the linkage area+param area */
#  define extra_stack_storage    56
# else
#  define extra_stack_storage    0
# endif
    /* Save all callee saved registers into a stackframe. */
    fprintf( outfile, "    \"\\tstwu %s, -%d(%s)\\n\"\n",ppc_reg[1], 48+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[3],  4+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[4],  8+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[5], 12+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[6], 16+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[7], 20+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[8], 24+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[9], 28+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[10],32+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[11],36+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[12],40+extra_stack_storage, ppc_reg[1]);

    /* r0 -> r3 (arg1) */
    fprintf( outfile, "    \"\\tmr %s, %s\\n\"\n", ppc_reg[3], ppc_reg[0]);

    /* save return address */
    fprintf( outfile, "    \"\\tmflr %s\\n\"\n", ppc_reg[0]);
    fprintf( outfile, "    \"\\tstw  %s, %d(%s)\\n\"\n", ppc_reg[0], 44+extra_stack_storage, ppc_reg[1]);

    /* Call the __wine_delay_load function, arg1 is arg1. */
    fprintf( outfile, "    \"\\tbl " __ASM_NAME("__wine_delay_load") "\\n\"\n");

    /* Load return value from call into ctr register */
    fprintf( outfile, "    \"\\tmtctr %s\\n\"\n", ppc_reg[3]);

    /* restore all saved registers and drop stackframe. */
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[3],  4+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[4],  8+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[5], 12+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[6], 16+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[7], 20+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[8], 24+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[9], 28+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[10],32+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[11],36+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tlwz  %s, %d(%s)\\n\"\n", ppc_reg[12],40+extra_stack_storage, ppc_reg[1]);

    /* Load return value from call into return register */
    fprintf( outfile, "    \"\\tlwz  %s,  %d(%s)\\n\"\n", ppc_reg[0], 44+extra_stack_storage, ppc_reg[1]);
    fprintf( outfile, "    \"\\tmtlr %s\\n\"\n", ppc_reg[0]);
    fprintf( outfile, "    \"\\taddi %s, %s, %d\\n\"\n", ppc_reg[1], ppc_reg[1],  48+extra_stack_storage);

    /* branch to ctr register. */
    fprintf( outfile, "    \"bctr\\n\"\n");
#else
#error You need to defined delayed import thunks for your architecture!
#endif
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
            fprintf( outfile, "    \"\\t" __ASM_FUNC("%s") "\\n\"\n", buffer );
            fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\"\n", buffer );
#if defined(__i386__)
            fprintf( outfile, "    \"\\tmovl $%d, %%eax\\n\"\n", (idx << 16) | j );
            fprintf( outfile, "    \"\\tjmp __wine_delay_load_asm\\n\"\n" );
#elif defined(__sparc__)
            fprintf( outfile, "    \"\\tset %d, %%g1\\n\"\n", (idx << 16) | j );
            fprintf( outfile, "    \"\\tb,a __wine_delay_load_asm\\n\"\n" );
#elif defined(__powerpc__)
#ifdef __APPLE__
            /* On Darwin we can use r0 and r2 */
            /* Upper part in r2 */
            fprintf( outfile, "    \"\\tlis %s, %d\\n\"\n", ppc_reg[2], idx);
            /* Lower part + r2 -> r0, Note we can't use r0 directly */
            fprintf( outfile, "    \"\\taddi %s, %s, %d\\n\"\n", ppc_reg[0], ppc_reg[2], j);
            fprintf( outfile, "    \"\\tb " __ASM_NAME("__wine_delay_load_asm") "\\n\"\n");
#else /* __APPLE__ */
            /* On linux we can't use r2 since r2 is not a scratch register (hold the TOC) */
            /* Save r13 on the stack */
            fprintf( outfile, "    \"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[13], ppc_reg[1]);
            /* Upper part in r13 */
            fprintf( outfile, "    \"\\tlis %s, %d\\n\"\n", ppc_reg[13], idx);
            /* Lower part + r13 -> r0, Note we can't use r0 directly */
            fprintf( outfile, "    \"\\taddi %s, %s, %d\\n\"\n", ppc_reg[0], ppc_reg[13], j);
            /* Restore r13 */
            fprintf( outfile, "    \"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[13], ppc_reg[1]);
            fprintf( outfile, "    \"\\taddic %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "    \"\\tb " __ASM_NAME("__wine_delay_load_asm") "\\n\"\n");
#endif /* __APPLE__ */
#else
#error You need to defined delayed import thunks for your architecture!
#endif
            output_function_size( outfile, name );
        }
        idx++;
    }
    output_function_size( outfile, delayed_import_loaders );

    fprintf( outfile, "\n    \".data\\n\\t.align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\"\n", delayed_import_thunks);
    pos = nb_delayed * 32;
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            ORDDEF *odp = dll_imports[i]->imports[j];
            const char *name = odp->name ? odp->name : odp->export_name;

            fprintf( outfile, "    \"\\t" __ASM_FUNC("%s") "\\n\"\n", name );
            fprintf( outfile, "    \"\\t.globl " __ASM_NAME("%s") "\\n\"\n", name );
            fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\\t\"", name );
#if defined(__i386__)
            if (strstr( name, "__wine_call_from_16" ))
                fprintf( outfile, "\".byte 0x2e\\n\\tjmp *(delay_imports+%d)\\n\\tnop\\n\"", pos );
            else
                fprintf( outfile, "\"jmp *(delay_imports+%d)\\n\\tmovl %%esi,%%esi\\n\"", pos );
#elif defined(__sparc__)
            if ( !UsePIC )
            {
                fprintf( outfile, "\"sethi %%hi(delay_imports+%d), %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"ld [%%g1+%%lo(delay_imports+%d)], %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"jmp %%g1\\n\\tnop\\n\"" );
            }
            else
            {
                /* Hmpf.  Stupid sparc assembler always interprets global variable
                   names as GOT offsets, so we have to do it the long way ... */
                fprintf( outfile, "\"save %%sp, -96, %%sp\\n\"" );
                fprintf( outfile, "\"0:\\tcall 1f\\n\\tnop\\n\"" );
                fprintf( outfile, "\"1:\\tsethi %%hi(delay_imports+%d-0b), %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"or %%g1, %%lo(delay_imports+%d-0b), %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"ld [%%g1+%%o7], %%g1\\n\\t\"" );
                fprintf( outfile, "\"jmp %%g1\\n\\trestore\\n\"" );
            }

#elif defined(__powerpc__)
            fprintf( outfile, "\t\"addi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "\t\"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[9], ppc_reg[1]);
            fprintf( outfile, "\t\"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "\t\"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[8], ppc_reg[1]);
            fprintf( outfile, "\t\"\\taddi %s, %s, -0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "\t\"\\tstw  %s, 0(%s)\\n\"\n",    ppc_reg[7], ppc_reg[1]);

            fprintf( outfile, "\t\"\\tlis %s, " ppc_high(__ASM_NAME("delay_imports") "+ %d") "\\n\"\n", ppc_reg[9], pos);
            fprintf( outfile, "\t\"\\tla  %s, " ppc_low (__ASM_NAME("delay_imports") "+ %d") "(%s)\\n\"\n", ppc_reg[8], pos, ppc_reg[9]);
            fprintf( outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n", ppc_reg[7], ppc_reg[8]);
            fprintf( outfile, "\t\"\\tmtctr %s\\n\"\n", ppc_reg[7]);

            fprintf( outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg[7], ppc_reg[1]);
            fprintf( outfile, "\t\"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg[8], ppc_reg[1]);
            fprintf( outfile, "\t\"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "\t\"\\tlwz  %s, 0(%s)\\n\"\n",   ppc_reg[9], ppc_reg[1]);
            fprintf( outfile, "\t\"\\taddi %s, %s, 0x4\\n\"\n", ppc_reg[1], ppc_reg[1]);
            fprintf( outfile, "\t\"\\tbctr\\n\"");
#else
#error You need to define delayed import thunks for your architecture!
#endif
            fprintf( outfile, "\n" );
            output_function_size( outfile, name );
        }
    }
    output_function_size( outfile, delayed_import_thunks );
    fprintf( outfile, "\".text\");\n" );
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif\n" );
    fprintf( outfile, "\n" );

 done:
    return nb_delayed;
}

/* output the import and delayed import tables of a Win32 module
 * returns number of DLLs exported in 'immediate' mode
 */
int output_imports( FILE *outfile, DLLSPEC *spec )
{
   output_delayed_imports( outfile, spec );
   return output_immediate_imports( outfile );
}
