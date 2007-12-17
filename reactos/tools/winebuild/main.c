/*
 * Main function
 *
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "build.h"

int UsePIC = 0;
int nb_lib_paths = 0;
int nb_errors = 0;
int display_warnings = 0;
int kill_at = 0;
int verbose = 0;
int save_temps = 0;
int link_ext_symbols = 0;

#ifdef __i386__
enum target_cpu target_cpu = CPU_x86;
#elif defined(__x86_64__)
enum target_cpu target_cpu = CPU_x86_64;
#elif defined(__sparc__)
enum target_cpu target_cpu = CPU_SPARC;
#elif defined(__ALPHA__)
enum target_cpu target_cpu = CPU_ALPHA;
#elif defined(__powerpc__)
enum target_cpu target_cpu = CPU_POWERPC;
#else
#error Unsupported CPU
#endif

#ifdef __APPLE__
enum target_platform target_platform = PLATFORM_APPLE;
#elif defined(_WINDOWS)
enum target_platform target_platform = PLATFORM_WINDOWS;
#else
enum target_platform target_platform = PLATFORM_UNSPECIFIED;
#endif

char **lib_path = NULL;

char *input_file_name = NULL;
char *spec_file_name = NULL;
FILE *output_file = NULL;
const char *output_file_name = NULL;
static const char *output_file_source_name;

char *as_command = NULL;
char *ld_command = NULL;
char *nm_command = NULL;

static int nb_res_files;
static char **res_files;

/* execution mode */
enum exec_mode_values
{
    MODE_NONE,
    MODE_DLL,
    MODE_EXE,
    MODE_DEF,
    MODE_RELAY16,
    MODE_RELAY32,
    MODE_PEDLL
};

static enum exec_mode_values exec_mode = MODE_NONE;

static const struct
{
    const char *name;
    enum target_cpu cpu;
} cpu_names[] =
{
    { "i386",    CPU_x86 },
    { "i486",    CPU_x86 },
    { "i586",    CPU_x86 },
    { "i686",    CPU_x86 },
    { "i786",    CPU_x86 },
    { "x86_64",  CPU_x86_64 },
    { "sparc",   CPU_SPARC },
    { "alpha",   CPU_ALPHA },
    { "powerpc", CPU_POWERPC }
};

static const struct
{
    const char *name;
    enum target_platform platform;
} platform_names[] =
{
    { "macos",   PLATFORM_APPLE },
    { "darwin",  PLATFORM_APPLE },
    { "windows", PLATFORM_WINDOWS },
    { "winnt",   PLATFORM_WINDOWS }
};

/* set the dll file name from the input file name */
static void set_dll_file_name( const char *name, DLLSPEC *spec )
{
    char *p;

    if (spec->file_name) return;

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    spec->file_name = xmalloc( strlen(name) + 5 );
    strcpy( spec->file_name, name );
    if ((p = strrchr( spec->file_name, '.' )))
    {
        if (!strcmp( p, ".spec" ) || !strcmp( p, ".def" )) *p = 0;
    }
}

/* set the dll subsystem */
static void set_subsystem( const char *subsystem, DLLSPEC *spec )
{
    char *major, *minor, *str = xstrdup( subsystem );

    if ((major = strchr( str, ':' ))) *major++ = 0;
    if (!strcmp( str, "native" )) spec->subsystem = IMAGE_SUBSYSTEM_NATIVE;
    else if (!strcmp( str, "windows" )) spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    else if (!strcmp( str, "console" )) spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    else fatal_error( "Invalid subsystem name '%s'\n", subsystem );
    if (major)
    {
        if ((minor = strchr( major, '.' )))
        {
            *minor++ = 0;
            spec->subsystem_minor = atoi( minor );
        }
        spec->subsystem_major = atoi( major );
    }
    free( str );
}

/* set the target CPU and platform */
static void set_target( const char *target )
{
    unsigned int i;
    char *p, *platform, *spec = xstrdup( target );

    /* target specification is in the form CPU-MANUFACTURER-OS or CPU-MANUFACTURER-KERNEL-OS */

    /* get the CPU part */

    if (!(p = strchr( spec, '-' ))) fatal_error( "Invalid target specification '%s'\n", target );
    *p++ = 0;
    for (i = 0; i < sizeof(cpu_names)/sizeof(cpu_names[0]); i++)
    {
        if (!strcmp( cpu_names[i].name, spec )) break;
    }
    if (i < sizeof(cpu_names)/sizeof(cpu_names[0])) target_cpu = cpu_names[i].cpu;
    else fatal_error( "Unrecognized CPU '%s'\n", spec );

    platform = p;
    if ((p = strrchr( p, '-' ))) platform = p + 1;

    /* get the OS part */

    target_platform = PLATFORM_UNSPECIFIED;  /* default value */
    for (i = 0; i < sizeof(platform_names)/sizeof(platform_names[0]); i++)
    {
        if (!strncmp( platform_names[i].name, platform, strlen(platform_names[i].name) ))
        {
            target_platform = platform_names[i].platform;
            break;
        }
    }

    free( spec );

    if (!as_command)
    {
        as_command = xmalloc( strlen(target) + sizeof("-as") );
        strcpy( as_command, target );
        strcat( as_command, "-as" );
    }
    if (!ld_command)
    {
        ld_command = xmalloc( strlen(target) + sizeof("-ld") );
        strcpy( ld_command, target );
        strcat( ld_command, "-ld" );
    }
    if (!nm_command)
    {
        nm_command = xmalloc( strlen(target) + sizeof("-nm") );
        strcpy( nm_command, target );
        strcat( nm_command, "-nm" );
    }
}

/* cleanup on program exit */
static void cleanup(void)
{
    if (output_file_name) unlink( output_file_name );
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call atexit functions */
}

/*******************************************************************
 *         command-line option handling
 */
static const char usage_str[] =
"Usage: winebuild [OPTIONS] [FILES]\n\n"
"Options:\n"
"       --as-cmd=AS          Command to use for assembling (default: as)\n"
"   -d, --delay-lib=LIB      Import the specified library in delayed mode\n"
"   -D SYM                   Ignored for C flags compatibility\n"
"   -e, --entry=FUNC         Set the DLL entry point function (default: DllMain)\n"
"   -E, --export=FILE        Export the symbols defined in the .spec or .def file\n"
"       --external-symbols   Allow linking to external symbols\n"
"   -f FLAGS                 Compiler flags (only -fPIC is supported)\n"
"   -F, --filename=DLLFILE   Set the DLL filename (default: from input file name)\n"
"   -h, --help               Display this help message\n"
"   -H, --heap=SIZE          Set the heap size for a Win16 dll\n"
"   -i, --ignore=SYM[,SYM]   Ignore specified symbols when resolving imports\n"
"   -I DIR                   Ignored for C flags compatibility\n"
"   -k, --kill-at            Kill stdcall decorations in generated .def files\n"
"   -K, FLAGS                Compiler flags (only -KPIC is supported)\n"
"       --ld-cmd=LD          Command to use for linking (default: ld)\n"
"   -l, --library=LIB        Import the specified library\n"
"   -L, --library-path=DIR   Look for imports libraries in DIR\n"
"   -M, --main-module=MODULE Set the name of the main module for a Win16 dll\n"
"       --nm-cmd=NM          Command to use to get undefined symbols (default: nm)\n"
"       --nxcompat=y|n       Set the NX compatibility flag (default: yes)\n"
"   -N, --dll-name=DLLNAME   Set the DLL name (default: from input file name)\n"
"   -o, --output=NAME        Set the output file name (default: stdout)\n"
"   -r, --res=RSRC.RES       Load resources from RSRC.RES\n"
"       --save-temps         Do not delete the generated intermediate files\n"
"       --subsystem=SUBSYS   Set the subsystem (one of native, windows, console)\n"
"       --target=TARGET      Specify target CPU and platform for cross-compiling\n"
"   -u, --undefined=SYMBOL   Add an undefined reference to SYMBOL when linking\n"
"   -v, --verbose            Display the programs invoked\n"
"       --version            Print the version and exit\n"
"   -w, --warnings           Turn on warnings\n"
"\nMode options:\n"
"       --dll                Build a .c file from a .spec or .def file\n"
"       --def                Build a .def file from a .spec file\n"
"       --exe                Build a .c file for an executable\n"
"       --relay16            Build the 16-bit relay assembly routines\n"
"       --relay32            Build the 32-bit relay assembly routines\n\n"
"       --pedll              Build a .c file for PE dll\n\n"
"The mode options are mutually exclusive; you must specify one and only one.\n\n";

enum long_options_values
{
    LONG_OPT_DLL = 1,
    LONG_OPT_DEF,
    LONG_OPT_EXE,
    LONG_OPT_ASCMD,
    LONG_OPT_EXTERNAL_SYMS,
    LONG_OPT_LDCMD,
    LONG_OPT_NMCMD,
    LONG_OPT_NXCOMPAT,
    LONG_OPT_RELAY16,
    LONG_OPT_RELAY32,
    LONG_OPT_SAVE_TEMPS,
    LONG_OPT_SUBSYSTEM,
    LONG_OPT_TARGET,
    LONG_OPT_VERSION,
    LONG_OPT_PEDLL
};

static const char short_options[] = "C:D:E:F:H:I:K:L:M:N:d:e:f:hi:kl:m:o:r:u:vw";

static const struct option long_options[] =
{
    { "dll",           0, 0, LONG_OPT_DLL },
    { "def",           0, 0, LONG_OPT_DEF },
    { "exe",           0, 0, LONG_OPT_EXE },
    { "as-cmd",        1, 0, LONG_OPT_ASCMD },
    { "external-symbols", 0, 0, LONG_OPT_EXTERNAL_SYMS },
    { "ld-cmd",        1, 0, LONG_OPT_LDCMD },
    { "nm-cmd",        1, 0, LONG_OPT_NMCMD },
    { "nxcompat",      1, 0, LONG_OPT_NXCOMPAT },
    { "relay16",       0, 0, LONG_OPT_RELAY16 },
    { "relay32",       0, 0, LONG_OPT_RELAY32 },
    { "save-temps",    0, 0, LONG_OPT_SAVE_TEMPS },
    { "subsystem",     1, 0, LONG_OPT_SUBSYSTEM },
    { "target",        1, 0, LONG_OPT_TARGET },
    { "version",       0, 0, LONG_OPT_VERSION },
    { "pedll",         1, 0, LONG_OPT_PEDLL },
    /* aliases for short options */
    { "delay-lib",     1, 0, 'd' },
    { "export",        1, 0, 'E' },
    { "entry",         1, 0, 'e' },
    { "filename",      1, 0, 'F' },
    { "help",          0, 0, 'h' },
    { "heap",          1, 0, 'H' },
    { "ignore",        1, 0, 'i' },
    { "kill-at",       0, 0, 'k' },
    { "library",       1, 0, 'l' },
    { "library-path",  1, 0, 'L' },
    { "main-module",   1, 0, 'M' },
    { "dll-name",      1, 0, 'N' },
    { "output",        1, 0, 'o' },
    { "res",           1, 0, 'r' },
    { "undefined",     1, 0, 'u' },
    { "verbose",       0, 0, 'v' },
    { "warnings",      0, 0, 'w' },
    { NULL,            0, 0, 0 }
};

static void usage( int exit_code )
{
    fprintf( stderr, "%s", usage_str );
    exit( exit_code );
}

static void set_exec_mode( enum exec_mode_values mode )
{
    if (exec_mode != MODE_NONE) usage(1);
    exec_mode = mode;
}

/* parse options from the argv array and remove all the recognized ones */
static char **parse_options( int argc, char **argv, DLLSPEC *spec )
{
    char *p;
    int optc;

    while ((optc = getopt_long( argc, argv, short_options, long_options, NULL )) != -1)
    {
        switch(optc)
        {
        case 'D':
            /* ignored */
            break;
        case 'E':
            spec_file_name = xstrdup( optarg );
            set_dll_file_name( optarg, spec );
            break;
        case 'F':
            spec->file_name = xstrdup( optarg );
            break;
        case 'H':
            if (!isdigit(optarg[0]))
                fatal_error( "Expected number argument with -H option instead of '%s'\n", optarg );
            spec->heap_size = atoi(optarg);
            if (spec->heap_size > 65535)
                fatal_error( "Invalid heap size %d, maximum is 65535\n", spec->heap_size );
            break;
        case 'I':
            /* ignored */
            break;
        case 'K':
            /* ignored, because cc generates correct code. */
            break;
        case 'L':
            lib_path = xrealloc( lib_path, (nb_lib_paths+1) * sizeof(*lib_path) );
            lib_path[nb_lib_paths++] = xstrdup( optarg );
            break;
        case 'M':
            spec->type = SPEC_WIN16;
            break;
        case 'N':
            spec->dll_name = xstrdup( optarg );
            break;
        case 'd':
            add_delayed_import( optarg );
            break;
        case 'e':
            spec->init_func = xstrdup( optarg );
            if ((p = strchr( spec->init_func, '@' ))) *p = 0;  /* kill stdcall decoration */
            break;
        case 'f':
            if (!strcmp( optarg, "PIC") || !strcmp( optarg, "pic")) UsePIC = 1;
            /* ignore all other flags */
            break;
        case 'h':
            usage(0);
            break;
        case 'i':
            {
                char *str = xstrdup( optarg );
                char *token = strtok( str, "," );
                while (token)
                {
                    add_ignore_symbol( token );
                    token = strtok( NULL, "," );
                }
                free( str );
            }
            break;
        case 'k':
            kill_at = 1;
            break;
        case 'l':
            add_import_dll( optarg, NULL );
            break;
        case 'o':
            {
                char *ext = strrchr( optarg, '.' );

                if (unlink( optarg ) == -1 && errno != ENOENT)
                    fatal_error( "Unable to create output file '%s'\n", optarg );
                if (ext && !strcmp( ext, ".o" ))
                {
                    output_file_source_name = get_temp_file_name( optarg, ".s" );
                    if (!(output_file = fopen( output_file_source_name, "w" )))
                        fatal_error( "Unable to create output file '%s'\n", optarg );
                }
                else
                {
                    if (!(output_file = fopen( optarg, "w" )))
                        fatal_error( "Unable to create output file '%s'\n", optarg );
                }
                output_file_name = xstrdup(optarg);
                atexit( cleanup );  /* make sure we remove the output file on exit */
            }
            break;
        case 'r':
            res_files = xrealloc( res_files, (nb_res_files+1) * sizeof(*res_files) );
            res_files[nb_res_files++] = xstrdup( optarg );
            break;
        case 'u':
            add_extra_ld_symbol( optarg );
            break;
        case 'v':
            verbose++;
            break;
        case 'w':
            display_warnings = 1;
            break;
        case LONG_OPT_DLL:
            set_exec_mode( MODE_DLL );
            break;
        case LONG_OPT_DEF:
            set_exec_mode( MODE_DEF );
            break;
        case LONG_OPT_EXE:
            set_exec_mode( MODE_EXE );
            if (!spec->subsystem) spec->subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
            break;
        case LONG_OPT_ASCMD:
            as_command = xstrdup( optarg );
            break;
        case LONG_OPT_EXTERNAL_SYMS:
            link_ext_symbols = 1;
            break;
        case LONG_OPT_LDCMD:
            ld_command = xstrdup( optarg );
            break;
        case LONG_OPT_NMCMD:
            nm_command = xstrdup( optarg );
            break;
        case LONG_OPT_NXCOMPAT:
            if (optarg[0] == 'n' || optarg[0] == 'N')
                spec->dll_characteristics &= ~IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
            break;
        case LONG_OPT_RELAY16:
            set_exec_mode( MODE_RELAY16 );
            break;
        case LONG_OPT_RELAY32:
            set_exec_mode( MODE_RELAY32 );
            break;
        case LONG_OPT_SAVE_TEMPS:
            save_temps = 1;
            break;
        case LONG_OPT_SUBSYSTEM:
            set_subsystem( optarg, spec );
            break;
        case LONG_OPT_TARGET:
            set_target( optarg );
            break;
        case LONG_OPT_VERSION:
            printf( "winebuild version " PACKAGE_VERSION "\n" );
            exit(0);
        case LONG_OPT_PEDLL:
            set_exec_mode( MODE_PEDLL );
            spec_file_name = xstrdup( optarg );
            set_dll_file_name( optarg, spec );
            break;
        case '?':
            usage(1);
            break;
        }
    }

    if (spec->file_name && !strchr( spec->file_name, '.' ))
        strcat( spec->file_name, exec_mode == MODE_EXE ? ".exe" : ".dll" );

    return &argv[optind];
}


/* load all specified resource files */
static void load_resources( char *argv[], DLLSPEC *spec )
{
    int i;
    char **ptr, **last;

    switch (spec->type)
    {
    case SPEC_WIN16:
        for (i = 0; i < nb_res_files; i++) load_res16_file( res_files[i], spec );
        break;

    case SPEC_WIN32:
        for (i = 0; i < nb_res_files; i++)
        {
            if (!load_res32_file( res_files[i], spec ))
                fatal_error( "%s is not a valid Win32 resource file\n", res_files[i] );
        }

        /* load any resource file found in the remaining arguments */
        for (ptr = last = argv; *ptr; ptr++)
        {
            if (!load_res32_file( *ptr, spec ))
                *last++ = *ptr; /* not a resource file, keep it in the list */
        }
        *last = NULL;
        break;
    }
}

/* add input files that look like import libs to the import list */
static void load_import_libs( char *argv[] )
{
    char **ptr, **last;

    for (ptr = last = argv; *ptr; ptr++)
    {
        if (strendswith( *ptr, ".def" ))
            add_import_dll( NULL, *ptr );
        else
            *last++ = *ptr; /* not an import dll, keep it in the list */
    }
    *last = NULL;
}

static int parse_input_file( DLLSPEC *spec )
{
    FILE *input_file = open_input_file( NULL, spec_file_name );
    char *extension = strrchr( spec_file_name, '.' );
    int result;

    spec->src_name = xstrdup( input_file_name );
    if (extension && !strcmp( extension, ".def" ))
        result = parse_def_file( input_file, spec );
    else
        result = parse_spec_file( input_file, spec );
    close_input_file( input_file );
    return result;
}


/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    DLLSPEC *spec = alloc_dll_spec();

#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );

    output_file = stdout;
    argv = parse_options( argc, argv, spec );

    switch(exec_mode)
    {
    case MODE_DLL:
        if (spec->subsystem != IMAGE_SUBSYSTEM_NATIVE)
            spec->characteristics |= IMAGE_FILE_DLL;
        load_resources( argv, spec );
        load_import_libs( argv );
        if (!spec_file_name) fatal_error( "missing .spec file\n" );
        if (!parse_input_file( spec )) break;
        switch (spec->type)
        {
            case SPEC_WIN16:
                fatal_error( "Win16 specs are not supported in ReactOS version of winebuild\n" );
                break;
            case SPEC_WIN32:
                read_undef_symbols( spec, argv );
                BuildSpec32File( spec );
                break;
            default: assert(0);
        }
        break;
    case MODE_EXE:
        if (spec->type == SPEC_WIN16) fatal_error( "Cannot build 16-bit exe files\n" );
	if (!spec->file_name) fatal_error( "executable must be named via the -F option\n" );
        load_resources( argv, spec );
        load_import_libs( argv );
        if (spec_file_name && !parse_input_file( spec )) break;
        read_undef_symbols( spec, argv );
        BuildSpec32File( spec );
        break;
    case MODE_DEF:
        if (argv[0]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
        if (spec->type == SPEC_WIN16) fatal_error( "Cannot yet build .def file for 16-bit dlls\n" );
        if (!spec_file_name) fatal_error( "missing .spec file\n" );
        if (!parse_input_file( spec )) break;
        BuildDef32File( spec );
        break;
    case MODE_RELAY16:
        fatal_error( "Win16 relays are not supported in ReactOS version of winebuild\n" );
        break;
    case MODE_RELAY32:
        fatal_error( "Win32 relays are not supported in ReactOS version of winebuild\n" );
        break;
    case MODE_PEDLL:
        if (argv[0]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
        if (!parse_input_file( spec )) break;
        BuildPedllFile( spec );
        break;
    default:
        usage(1);
        break;
    }
    if (nb_errors) exit(1);
    if (output_file_name)
    {
        if (fclose( output_file ) < 0) fatal_perror( "fclose" );
        if (output_file_source_name) assemble_file( output_file_source_name, output_file_name );
        output_file_name = NULL;
    }
    return 0;
}
