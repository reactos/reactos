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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "build.h"

ORDDEF *EntryPoints[MAX_ORDINALS];
ORDDEF *Ordinals[MAX_ORDINALS];
ORDDEF *Names[MAX_ORDINALS];

SPEC_MODE SpecMode = SPEC_MODE_DLL;
SPEC_TYPE SpecType = SPEC_WIN32;

int Base = MAX_ORDINALS;
int Limit = 0;
int DLLHeapSize = 0;
int UsePIC = 0;
int stack_size = 0;
int nb_entry_points = 0;
int nb_names = 0;
int nb_debug_channels = 0;
int nb_lib_paths = 0;
int nb_errors = 0;
int display_warnings = 0;
int kill_at = 0;

/* we only support relay debugging on i386 */
#if defined(__i386__) && !defined(NO_TRACE_MSGS)
int debugging = 1;
#else
int debugging = 0;
#endif

char *owner_name = NULL;
char *dll_name = NULL;
char *dll_file_name = NULL;
const char *init_func = NULL;
char **debug_channels = NULL;
char **lib_path = NULL;

char *input_file_name = NULL;
const char *output_file_name = NULL;

static FILE *input_file;
static FILE *output_file;
static const char *current_src_dir;
static int nb_res_files;
static char **res_files;

/* execution mode */
enum exec_mode_values
{
    MODE_NONE,
    MODE_SPEC,
    MODE_EXE,
    MODE_DEF,
    MODE_DEBUG,
    MODE_RELAY16,
    MODE_RELAY32
};

static enum exec_mode_values exec_mode = MODE_NONE;

/* set the dll file name from the input file name */
static void set_dll_file_name( const char *name )
{
    char *p;

    if (dll_file_name) return;

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    dll_file_name = xmalloc( strlen(name) + 5 );
    strcpy( dll_file_name, name );
    if ((p = strrchr( dll_file_name, '.' )) && !strcmp( p, ".spec" )) *p = 0;
    if (!strchr( dll_file_name, '.' )) strcat( dll_file_name, ".dll" );
}

/* cleanup on program exit */
static void cleanup(void)
{
    if (output_file_name) unlink( output_file_name );
}


/*******************************************************************
 *         command-line option handling
 */
static const char usage_str[] =
"Usage: winebuild [OPTIONS] [FILES]\n\n"
"Options:\n"
"    -C --source-dir=DIR     Look for source files in DIR\n"
"    -d --delay-lib=LIB      Import the specified library in delayed mode\n"
"    -D SYM                  Ignored for C flags compatibility\n"
"    -e --entry=FUNC         Set the DLL entry point function (default: DllMain)\n"
"    -f FLAGS                Compiler flags (only -fPIC is supported)\n"
"    -F --filename=DLLFILE   Set the DLL filename (default: from input file name)\n"
"    -h --help               Display this help message\n"
"    -H --heap=SIZE          Set the heap size for a Win16 dll\n"
"    -i --ignore=SYM[,SYM]   Ignore specified symbols when resolving imports\n"
"    -I DIR                  Ignored for C flags compatibility\n"
"    -k --kill-at            Kill stdcall decorations in generated .def files\n"
"    -K FLAGS                Compiler flags (only -KPIC is supported)\n"
"    -l --library=LIB        Import the specified library\n"
"    -L --library-path=DIR   Look for imports libraries in DIR\n"
"    -m --exe-mode=MODE      Set the executable mode (cui|gui|cuiw|guiw)\n"
"    -M --main-module=MODULE Set the name of the main module for a Win16 dll\n"
"    -N --dll-name=DLLNAME   Set the DLL name (default: from input file name)\n"
"    -o --output=NAME        Set the output file name (default: stdout)\n"
"    -r --res=RSRC.RES       Load resources from RSRC.RES\n"
"       --version            Print the version and exit\n"
"    -w --warnings           Turn on warnings\n"
"\nMode options:\n"
"       --spec=FILE.SPEC     Build a .c file from a spec file\n"
"       --def=FILE.SPEC      Build a .def file from a spec file\n"
"       --exe=NAME           Build a .c file for the named executable\n"
"       --debug [FILES]      Build a .c file with the debug channels declarations\n"
"       --relay16            Build the 16-bit relay assembly routines\n"
"       --relay32            Build the 32-bit relay assembly routines\n\n"
"The mode options are mutually exclusive; you must specify one and only one.\n\n";

enum long_options_values
{
    LONG_OPT_SPEC = 1,
    LONG_OPT_DEF,
    LONG_OPT_EXE,
    LONG_OPT_DEBUG,
    LONG_OPT_RELAY16,
    LONG_OPT_RELAY32,
    LONG_OPT_VERSION
};

static const char short_options[] = "C:D:F:H:I:K:L:M:N:d:e:f:hi:kl:m:o:r:w";

static const struct option long_options[] =
{
    { "spec",     1, 0, LONG_OPT_SPEC },
    { "def",      1, 0, LONG_OPT_DEF },
    { "exe",      1, 0, LONG_OPT_EXE },
    { "debug",    0, 0, LONG_OPT_DEBUG },
    { "relay16",  0, 0, LONG_OPT_RELAY16 },
    { "relay32",  0, 0, LONG_OPT_RELAY32 },
    { "version",  0, 0, LONG_OPT_VERSION },
    /* aliases for short options */
    { "source-dir",    1, 0, 'C' },
    { "delay-lib",     1, 0, 'd' },
    { "entry",         1, 0, 'e' },
    { "filename",      1, 0, 'F' },
    { "help",          0, 0, 'h' },
    { "heap",          1, 0, 'H' },
    { "ignore",        1, 0, 'i' },
    { "kill-at",       0, 0, 'k' },
    { "library",       1, 0, 'l' },
    { "library-path",  1, 0, 'L' },
    { "exe-mode",      1, 0, 'm' },
    { "main-module",   1, 0, 'M' },
    { "dll-name",      1, 0, 'N' },
    { "output",        1, 0, 'o' },
    { "res",           1, 0, 'r' },
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
static char **parse_options( int argc, char **argv )
{
    char *p;
    int optc;

    while ((optc = getopt_long( argc, argv, short_options, long_options, NULL )) != -1)
    {
        switch(optc)
        {
        case 'C':
            current_src_dir = optarg;
            break;
        case 'D':
            /* ignored */
            break;
        case 'F':
            dll_file_name = xstrdup( optarg );
            break;
        case 'H':
            if (!isdigit(optarg[0]))
                fatal_error( "Expected number argument with -H option instead of '%s'\n", optarg );
            DLLHeapSize = atoi(optarg);
            if (DLLHeapSize > 65535)
                fatal_error( "Invalid heap size %d, maximum is 65535\n", DLLHeapSize );
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
            owner_name = xstrdup( optarg );
            SpecType = SPEC_WIN16;
            break;
        case 'N':
            dll_name = xstrdup( optarg );
            break;
        case 'd':
            add_import_dll( optarg, 1 );
            break;
        case 'e':
            init_func = xstrdup( optarg );
            if ((p = strchr( init_func, '@' ))) *p = 0;  /* kill stdcall decoration */
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
            add_import_dll( optarg, 0 );
            break;
        case 'm':
            if (!strcmp( optarg, "gui" )) SpecMode = SPEC_MODE_GUIEXE;
            else if (!strcmp( optarg, "cui" )) SpecMode = SPEC_MODE_CUIEXE;
            else if (!strcmp( optarg, "guiw" )) SpecMode = SPEC_MODE_GUIEXE_UNICODE;
            else if (!strcmp( optarg, "cuiw" )) SpecMode = SPEC_MODE_CUIEXE_UNICODE;
            else usage(1);
            break;
        case 'o':
            if (unlink( optarg ) == -1 && errno != ENOENT)
                fatal_error( "Unable to create output file '%s'\n", optarg );
            if (!(output_file = fopen( optarg, "w" )))
                fatal_error( "Unable to create output file '%s'\n", optarg );
            output_file_name = xstrdup(optarg);
            atexit( cleanup );  /* make sure we remove the output file on exit */
            break;
        case 'r':
            res_files = xrealloc( res_files, (nb_res_files+1) * sizeof(*res_files) );
            res_files[nb_res_files++] = xstrdup( optarg );
            break;
        case 'w':
            display_warnings = 1;
            break;
        case LONG_OPT_SPEC:
            set_exec_mode( MODE_SPEC );
            input_file = open_input_file( NULL, optarg );
            set_dll_file_name( optarg );
            break;
        case LONG_OPT_DEF:
            set_exec_mode( MODE_DEF );
            input_file = open_input_file( NULL, optarg );
            set_dll_file_name( optarg );
            break;
        case LONG_OPT_EXE:
            set_exec_mode( MODE_EXE );
            if ((p = strrchr( optarg, '/' ))) p++;
            else p = optarg;
            dll_file_name = xmalloc( strlen(p) + 5 );
            strcpy( dll_file_name, p );
            if (!strchr( dll_file_name, '.' )) strcat( dll_file_name, ".exe" );
            if (SpecMode == SPEC_MODE_DLL) SpecMode = SPEC_MODE_GUIEXE;
            break;
        case LONG_OPT_DEBUG:
            set_exec_mode( MODE_DEBUG );
            break;
        case LONG_OPT_RELAY16:
            set_exec_mode( MODE_RELAY16 );
            break;
        case LONG_OPT_RELAY32:
            set_exec_mode( MODE_RELAY32 );
            break;
        case LONG_OPT_VERSION:
            printf( "winebuild version " PACKAGE_VERSION "\n" );
            exit(0);
        case '?':
            usage(1);
            break;
        }
    }
    return &argv[optind];
}


/* load all specified resource files */
static void load_resources( char *argv[] )
{
    int i;
    char **ptr, **last;

    switch (SpecType)
    {
    case SPEC_WIN16:
        for (i = 0; i < nb_res_files; i++) load_res16_file( res_files[i] );
        break;

    case SPEC_WIN32:
        for (i = 0; i < nb_res_files; i++)
        {
            if (!load_res32_file( res_files[i] ))
                fatal_error( "%s is not a valid Win32 resource file\n", res_files[i] );
        }

        /* load any resource file found in the remaining arguments */
        for (ptr = last = argv; *ptr; ptr++)
        {
            if (!load_res32_file( *ptr ))
                *last++ = *ptr; /* not a resource file, keep it in the list */
        }
        *last = NULL;
        break;
    }
}

/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    output_file = stdout;
    argv = parse_options( argc, argv );

    switch(exec_mode)
    {
    case MODE_SPEC:
        load_resources( argv );
        if (!ParseTopLevel( input_file )) break;
        switch (SpecType)
        {
            case SPEC_WIN16:
#if defined(WIN32)
                fatal_error( "Win16 specs are not supported in ReactOS version of winebuild\n" );
#else
                if (argv[0])
                    fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
                BuildSpec16File( output_file );
#endif
                break;
            case SPEC_WIN32:
                read_undef_symbols( argv );
                BuildSpec32File( output_file );
                break;
            default: assert(0);
        }
        break;
    case MODE_EXE:
        if (SpecType == SPEC_WIN16) fatal_error( "Cannot build 16-bit exe files\n" );
        load_resources( argv );
        read_undef_symbols( argv );
        BuildSpec32File( output_file );
        break;
    case MODE_DEF:
        if (argv[0]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
        if (SpecType == SPEC_WIN16) fatal_error( "Cannot yet build .def file for 16-bit dlls\n" );
        if (!ParseTopLevel( input_file )) break;
        BuildDef32File( output_file );
        break;
    case MODE_DEBUG:
        BuildDebugFile( output_file, current_src_dir, argv );
        break;
    case MODE_RELAY16:
#if defined(WIN32)
        fatal_error( "Win16 relays are not supported in ReactOS version of winebuild\n" );
#else
        if (argv[0]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
        BuildRelays16( output_file );
#endif
        break;
    case MODE_RELAY32:
#if defined(WIN32)
        fatal_error( "Win32 relays are not supported in ReactOS version of winebuild\n" );
#else
        if (argv[0]) fatal_error( "file argument '%s' not allowed in this mode\n", argv[0] );
        BuildRelays32( output_file );
#endif
        break;
    default:
        usage(1);
        break;
    }
    if (nb_errors) exit(1);
    if (output_file_name)
    {
        fclose( output_file );
        output_file_name = NULL;
    }
    return 0;
}
