/*
 * Small utility functions for winebuild
 *
 * Copyright 2000 Alexandre Julliard
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "build.h"

#define MAX_TMP_FILES 8
static const char *tmp_files[MAX_TMP_FILES];
static unsigned int nb_tmp_files;

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
    { "powerpc", CPU_POWERPC },
    { "arm", CPU_ARM }
};

/* atexit handler to clean tmp files */
static void cleanup_tmp_files(void)
{
    unsigned int i;
    for (i = 0; i < MAX_TMP_FILES; i++) if (tmp_files[i]) unlink( tmp_files[i] );
}


void *xmalloc (size_t size)
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}

void *xrealloc (void *ptr, size_t size)
{
    void *res = realloc (ptr, size);
    if (size && res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}

char *xstrdup( const char *str )
{
    char *res = strdup( str );
    if (!res)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}

char *strupper(char *s)
{
    char *p;
    for (p = s; *p; p++) *p = toupper(*p);
    return s;
}

int strendswith(const char* str, const char* end)
{
    int l = strlen(str);
    int m = strlen(end);
    return l >= m && strcmp(str + l - m, end) == 0;
}

void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    else fprintf( stderr, "winebuild: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}

void fatal_perror( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    vfprintf( stderr, msg, valist );
    perror( " " );
    va_end( valist );
    exit(1);
}

void error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    vfprintf( stderr, msg, valist );
    va_end( valist );
    nb_errors++;
}

void warning( const char *msg, ... )
{
    va_list valist;

    if (!display_warnings) return;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    fprintf( stderr, "warning: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
}

int output( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = vfprintf( output_file, format, valist );
    va_end( valist );
    if (ret < 0) fatal_perror( "Output error" );
    return ret;
}

/* find a build tool in the path, trying the various names */
static char *find_tool( const char * const *names )
{
    static char **dirs;
    static unsigned int count, maxlen;

    char *p, *file;
    unsigned int i, len;
    struct stat st;

    if (!dirs)
    {
        char *path;

        /* split the path in directories */

        if (!getenv( "PATH" )) return NULL;
        path = xstrdup( getenv( "PATH" ));
        for (p = path, count = 2; *p; p++) if (*p == ':') count++;
        dirs = xmalloc( count * sizeof(*dirs) );
        count = 0;
        dirs[count++] = p = path;
        while (*p)
        {
            while (*p && *p != ':') p++;
            if (!*p) break;
            *p++ = 0;
            dirs[count++] = p;
        }
        for (i = 0; i < count; i++) maxlen = max( maxlen, strlen(dirs[i])+2 );
    }

    while (*names)
    {
        len = strlen(*names) + sizeof(EXEEXT) + 1;
        file = xmalloc( maxlen + len );

        for (i = 0; i < count; i++)
        {
            strcpy( file, dirs[i] );
            p = file + strlen(file);
            if (p == file) *p++ = '.';
            if (p[-1] != '/') *p++ = '/';
            strcpy( p, *names );
            strcat( p, EXEEXT );

            if (!stat( file, &st ) && S_ISREG(st.st_mode) && (st.st_mode & 0111)) return file;
        }
        free( file );
        names++;
    }
    return NULL;
}

const char *get_as_command(void)
{
    if (!as_command)
    {
        if (target_alias)
        {
            as_command = xmalloc( strlen(target_alias) + sizeof("-as") );
            strcpy( as_command, target_alias );
            strcat( as_command, "-as" );
        }
        else
        {
            static const char * const commands[] = { "gas", "as", NULL };
            if (!(as_command = find_tool( commands ))) as_command = xstrdup("as");
        }

        if (force_pointer_size)
        {
            const char *args = (target_platform == PLATFORM_APPLE) ?
                ((force_pointer_size == 8) ? " -arch x86_64" : " -arch i386") :
                ((force_pointer_size == 8) ? " --64" : " --32");
            as_command = xrealloc( as_command, strlen(as_command) + strlen(args) + 1 );
            strcat( as_command, args );
        }
    }
    return as_command;
}

const char *get_ld_command(void)
{
    if (!ld_command)
    {
        if (target_alias)
        {
            ld_command = xmalloc( strlen(target_alias) + sizeof("-ld") );
            strcpy( ld_command, target_alias );
            strcat( ld_command, "-ld" );
        }
        else
        {
            static const char * const commands[] = { "ld", "gld", NULL };
            if (!(ld_command = find_tool( commands ))) ld_command = xstrdup("ld");
        }

        if (force_pointer_size)
        {
            const char *args;

            switch (target_platform)
            {
            case PLATFORM_APPLE:
                args = (force_pointer_size == 8) ? " -arch x86_64" : " -arch i386";
                break;
            case PLATFORM_FREEBSD:
                args = (force_pointer_size == 8) ? " -m elf_x86_64" : " -m elf_i386_fbsd";
                break;
            default:
                args = (force_pointer_size == 8) ? " -m elf_x86_64" : " -m elf_i386";
                break;
            }
            ld_command = xrealloc( ld_command, strlen(ld_command) + strlen(args) + 1 );
            strcat( ld_command, args );
        }
    }
    return ld_command;
}

const char *get_nm_command(void)
{
    if (!nm_command)
    {
        if (target_alias)
        {
            nm_command = xmalloc( strlen(target_alias) + sizeof("-nm") );
            strcpy( nm_command, target_alias );
            strcat( nm_command, "-nm" );
        }
        else
        {
            static const char * const commands[] = { "nm", "gnm", NULL };
            if (!(nm_command = find_tool( commands ))) nm_command = xstrdup("nm");
        }
    }
    return nm_command;
}

const char *get_windres_command(void)
{
    static char *windres_command;

    if (!windres_command)
    {
        if (target_alias)
        {
            windres_command = xmalloc( strlen(target_alias) + sizeof("-windres") );
            strcpy( windres_command, target_alias );
            strcat( windres_command, "-windres" );
        }
        else
        {
            static const char * const commands[] = { "windres", NULL };
            if (!(windres_command = find_tool( commands ))) windres_command = xstrdup("windres");
        }
    }
    return windres_command;
}

/* get a name for a temp file, automatically cleaned up on exit */
char *get_temp_file_name( const char *prefix, const char *suffix )
{
    char *name;
    const char *ext;
    int fd;

    assert( nb_tmp_files < MAX_TMP_FILES );
    if (!nb_tmp_files && !save_temps) atexit( cleanup_tmp_files );

    if (!prefix || !prefix[0]) prefix = "winebuild";
    if (!suffix) suffix = "";
    if (!(ext = strchr( prefix, '.' ))) ext = prefix + strlen(prefix);
    name = xmalloc( sizeof("/tmp/") + (ext - prefix) + sizeof(".XXXXXX") + strlen(suffix) );
    strcpy( name, "/tmp/" );
    memcpy( name + 5, prefix, ext - prefix );
    strcpy( name + 5 + (ext - prefix), ".XXXXXX" );
    strcat( name, suffix );

    /* first try without the /tmp/ prefix */
    if ((fd = mkstemps( name + 5, strlen(suffix) )) != -1)
        name += 5;
    else if ((fd = mkstemps( name, strlen(suffix) )) == -1)
        fatal_error( "could not generate a temp file\n" );

    close( fd );
    tmp_files[nb_tmp_files++] = name;
    return name;
}

/*******************************************************************
 *         buffer management
 *
 * Function for reading from/writing to a memory buffer.
 */

int byte_swapped = 0;
const char *input_buffer_filename;
const unsigned char *input_buffer;
size_t input_buffer_pos;
size_t input_buffer_size;
unsigned char *output_buffer;
size_t output_buffer_pos;
size_t output_buffer_size;

static void check_output_buffer_space( size_t size )
{
    if (output_buffer_pos + size >= output_buffer_size)
    {
        output_buffer_size = max( output_buffer_size * 2, output_buffer_pos + size );
        output_buffer = xrealloc( output_buffer, output_buffer_size );
    }
}

void init_input_buffer( const char *file )
{
    int fd;
    struct stat st;

    if ((fd = open( file, O_RDONLY | O_BINARY )) == -1) fatal_perror( "Cannot open %s", file );
    if ((fstat( fd, &st ) == -1)) fatal_perror( "Cannot stat %s", file );
    if (!st.st_size) fatal_error( "%s is an empty file\n", file );
#ifdef	HAVE_MMAP
    if ((input_buffer = mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0 )) == (void*)-1)
#endif
    {
        unsigned char *buffer = xmalloc( st.st_size );
        if (read( fd, buffer, st.st_size ) != st.st_size) fatal_error( "Cannot read %s\n", file );
        input_buffer = buffer;
    }
    close( fd );
    input_buffer_filename = xstrdup( file );
    input_buffer_size = st.st_size;
    input_buffer_pos = 0;
    byte_swapped = 0;
}

void init_output_buffer(void)
{
    output_buffer_size = 1024;
    output_buffer_pos = 0;
    output_buffer = xmalloc( output_buffer_size );
}

void flush_output_buffer(void)
{
    if (fwrite( output_buffer, 1, output_buffer_pos, output_file ) != output_buffer_pos)
        fatal_error( "Error writing to %s\n", output_file_name );
    free( output_buffer );
}

unsigned char get_byte(void)
{
    if (input_buffer_pos >= input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
    return input_buffer[input_buffer_pos++];
}

unsigned short get_word(void)
{
    unsigned short ret;

    if (input_buffer_pos + sizeof(ret) > input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
    memcpy( &ret, input_buffer + input_buffer_pos, sizeof(ret) );
    if (byte_swapped) ret = (ret << 8) | (ret >> 8);
    input_buffer_pos += sizeof(ret);
    return ret;
}

unsigned int get_dword(void)
{
    unsigned int ret;

    if (input_buffer_pos + sizeof(ret) > input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
    memcpy( &ret, input_buffer + input_buffer_pos, sizeof(ret) );
    if (byte_swapped)
        ret = ((ret << 24) | ((ret << 8) & 0x00ff0000) | ((ret >> 8) & 0x0000ff00) | (ret >> 24));
    input_buffer_pos += sizeof(ret);
    return ret;
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
    if (get_ptr_size() == 8) put_qword( val );
    else put_dword( val );
}

void align_output( unsigned int align )
{
    size_t size = align - (output_buffer_pos % align);

    if (size == align) return;
    check_output_buffer_space( size );
    memset( output_buffer + output_buffer_pos, 0, size );
    output_buffer_pos += size;
}

/* output a standard header for generated files */
void output_standard_file_header(void)
{
    if (spec_file_name)
        output( "/* File generated automatically from %s; do not edit! */\n", spec_file_name );
    else
        output( "/* File generated automatically; do not edit! */\n" );
    output( "/* This file can be copied, modified and distributed without restriction. */\n\n" );
}

/* dump a byte stream into the assembly code */
void dump_bytes( const void *buffer, unsigned int size )
{
    unsigned int i;
    const unsigned char *ptr = buffer;

    if (!size) return;
    output( "\t.byte " );
    for (i = 0; i < size - 1; i++, ptr++)
    {
        if ((i % 16) == 15) output( "0x%02x\n\t.byte ", *ptr );
        else output( "0x%02x,", *ptr );
    }
    output( "0x%02x\n", *ptr );
}


/*******************************************************************
 *         open_input_file
 *
 * Open a file in the given srcdir and set the input_file_name global variable.
 */
FILE *open_input_file( const char *srcdir, const char *name )
{
    char *fullname;
    FILE *file = fopen( name, "r" );

    if (!file && srcdir)
    {
        fullname = xmalloc( strlen(srcdir) + strlen(name) + 2 );
        strcpy( fullname, srcdir );
        strcat( fullname, "/" );
        strcat( fullname, name );
        file = fopen( fullname, "r" );
    }
    else fullname = xstrdup( name );

    if (!file) fatal_error( "Cannot open file '%s'\n", fullname );
    input_file_name = fullname;
    current_line = 1;
    return file;
}


/*******************************************************************
 *         close_input_file
 *
 * Close the current input file (must have been opened with open_input_file).
 */
void close_input_file( FILE *file )
{
    fclose( file );
    free( input_file_name );
    input_file_name = NULL;
    current_line = 0;
}


/*******************************************************************
 *         remove_stdcall_decoration
 *
 * Remove a possible @xx suffix from a function name.
 * Return the numerical value of the suffix, or -1 if none.
 */
int remove_stdcall_decoration( char *name )
{
    char *p, *end = strrchr( name, '@' );
    if (!end || !end[1] || end == name) return -1;
    /* make sure all the rest is digits */
    for (p = end + 1; *p; p++) if (!isdigit(*p)) return -1;
    *end = 0;
    return atoi( end + 1 );
}


/*******************************************************************
 *         assemble_file
 *
 * Run a file through the assembler.
 */
void assemble_file( const char *src_file, const char *obj_file )
{
    const char *prog = get_as_command();
    char *cmd;
    int err;

    cmd = xmalloc( strlen(prog) + strlen(obj_file) + strlen(src_file) + 6 );
    sprintf( cmd, "%s -o %s %s", prog, obj_file, src_file );
    if (verbose) fprintf( stderr, "%s\n", cmd );
    err = system( cmd );
    if (err) fatal_error( "%s failed with status %d\n", prog, err );
    free( cmd );
}


/*******************************************************************
 *         alloc_dll_spec
 *
 * Create a new dll spec file descriptor
 */
DLLSPEC *alloc_dll_spec(void)
{
    DLLSPEC *spec;

    spec = xmalloc( sizeof(*spec) );
    spec->file_name          = NULL;
    spec->dll_name           = NULL;
    spec->init_func          = NULL;
    spec->main_module        = NULL;
    spec->type               = SPEC_WIN32;
    spec->base               = MAX_ORDINALS;
    spec->limit              = 0;
    spec->stack_size         = 0;
    spec->heap_size          = 0;
    spec->nb_entry_points    = 0;
    spec->alloc_entry_points = 0;
    spec->nb_names           = 0;
    spec->nb_resources       = 0;
    spec->characteristics    = IMAGE_FILE_EXECUTABLE_IMAGE;
    if (get_ptr_size() > 4)
        spec->characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    else
        spec->characteristics |= IMAGE_FILE_32BIT_MACHINE;
    spec->dll_characteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    spec->subsystem          = 0;
    spec->subsystem_major    = 4;
    spec->subsystem_minor    = 0;
    spec->entry_points       = NULL;
    spec->names              = NULL;
    spec->ordinals           = NULL;
    spec->resources          = NULL;
    return spec;
}


/*******************************************************************
 *         free_dll_spec
 *
 * Free dll spec file descriptor
 */
void free_dll_spec( DLLSPEC *spec )
{
    int i;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        free( odp->name );
        free( odp->export_name );
        free( odp->link_name );
    }
    free( spec->file_name );
    free( spec->dll_name );
    free( spec->init_func );
    free( spec->entry_points );
    free( spec->names );
    free( spec->ordinals );
    free( spec->resources );
    free( spec );
}


/*******************************************************************
 *         make_c_identifier
 *
 * Map a string to a valid C identifier.
 */
const char *make_c_identifier( const char *str )
{
    static char buffer[256];
    char *p;

    for (p = buffer; *str && p < buffer+sizeof(buffer)-1; p++, str++)
    {
        if (isalnum(*str)) *p = *str;
        else *p = '_';
    }
    *p = 0;
    return buffer;
}


/*******************************************************************
 *         get_stub_name
 *
 * Generate an internal name for a stub entry point.
 */
const char *get_stub_name( const ORDDEF *odp, const DLLSPEC *spec )
{
    static char buffer[256];
    if (odp->name || odp->export_name)
    {
        char *p;
        sprintf( buffer, "__wine_stub_%s", odp->name ? odp->name : odp->export_name );
        /* make sure name is a legal C identifier */
        for (p = buffer; *p; p++) if (!isalnum(*p) && *p != '_') break;
        if (!*p) return buffer;
    }
    sprintf( buffer, "__wine_stub_%s_%d", make_c_identifier(spec->file_name), odp->ordinal );
    return buffer;
}

/* parse a cpu name and return the corresponding value */
enum target_cpu get_cpu_from_name( const char *name )
{
    unsigned int i;

    for (i = 0; i < sizeof(cpu_names)/sizeof(cpu_names[0]); i++)
        if (!strcmp( cpu_names[i].name, name )) return cpu_names[i].cpu;
    return -1;
}

/*****************************************************************
 *  Function:    get_alignment
 *
 *  Description:
 *    According to the info page for gas, the .align directive behaves
 * differently on different systems.  On some architectures, the
 * argument of a .align directive is the number of bytes to pad to, so
 * to align on an 8-byte boundary you'd say
 *     .align 8
 * On other systems, the argument is "the number of low-order zero bits
 * that the location counter must have after advancement."  So to
 * align on an 8-byte boundary you'd say
 *     .align 3
 *
 * The reason gas is written this way is that it's trying to mimick
 * native assemblers for the various architectures it runs on.  gas
 * provides other directives that work consistently across
 * architectures, but of course we want to work on all arches with or
 * without gas.  Hence this function.
 *
 *
 *  Parameters:
 *    align  --  the number of bytes to align to. Must be a power of 2.
 */
unsigned int get_alignment(unsigned int align)
{
    unsigned int n;

    assert( !(align & (align - 1)) );

    switch(target_cpu)
    {
    case CPU_x86:
    case CPU_x86_64:
    case CPU_SPARC:
    case CPU_ARM:
        if (target_platform != PLATFORM_APPLE) return align;
        /* fall through */
    case CPU_POWERPC:
    case CPU_ALPHA:
        n = 0;
        while ((1u << n) != align) n++;
        return n;
    }
    /* unreached */
    assert(0);
    return 0;
}

/* return the page size for the target CPU */
unsigned int get_page_size(void)
{
    switch(target_cpu)
    {
    case CPU_x86:     return 4096;
    case CPU_x86_64:  return 4096;
    case CPU_POWERPC: return 4096;
    case CPU_ARM:     return 4096;
    case CPU_SPARC:   return 8192;
    case CPU_ALPHA:   return 8192;
    }
    /* unreached */
    assert(0);
    return 0;
}

/* return the size of a pointer on the target CPU */
unsigned int get_ptr_size(void)
{
    switch(target_cpu)
    {
    case CPU_x86:
    case CPU_POWERPC:
    case CPU_SPARC:
    case CPU_ALPHA:
    case CPU_ARM:
        return 4;
    case CPU_x86_64:
        return 8;
    }
    /* unreached */
    assert(0);
    return 0;
}

/* return the assembly name for a C symbol */
const char *asm_name( const char *sym )
{
    static char buffer[256];

    switch (target_platform)
    {
    case PLATFORM_APPLE:
    case PLATFORM_WINDOWS:
        if (sym[0] == '.' && sym[1] == 'L') return sym;
        buffer[0] = '_';
        strcpy( buffer + 1, sym );
        return buffer;
    default:
        return sym;
    }
}

/* return an assembly function declaration for a C function name */
const char *func_declaration( const char *func )
{
    static char buffer[256];

    switch (target_platform)
    {
    case PLATFORM_APPLE:
        return "";
    case PLATFORM_WINDOWS:
        sprintf( buffer, ".def _%s; .scl 2; .type 32; .endef", func );
        break;
    default:
        switch(target_cpu)
        {
        case CPU_ARM:
            sprintf( buffer, ".type %s,%%function", func );
            break;
        default:
            sprintf( buffer, ".type %s,@function", func );
            break;
        }
        break;
    }
    return buffer;
}

/* output a size declaration for an assembly function */
void output_function_size( const char *name )
{
    switch (target_platform)
    {
    case PLATFORM_APPLE:
    case PLATFORM_WINDOWS:
        break;
    default:
        output( "\t.size %s, .-%s\n", name, name );
        break;
    }
}

/* output the GNU note for non-exec stack */
void output_gnu_stack_note(void)
{
    switch (target_platform)
    {
    case PLATFORM_WINDOWS:
    case PLATFORM_APPLE:
        break;
    default:
        switch(target_cpu)
        {
        case CPU_ARM:
            output( "\t.section .note.GNU-stack,\"\",%%progbits\n" );
            break;
        default:
            output( "\t.section .note.GNU-stack,\"\",@progbits\n" );
            break;
        }
        break;
    }
}

/* return a global symbol declaration for an assembly symbol */
const char *asm_globl( const char *func )
{
    static char buffer[256];

    switch (target_platform)
    {
    case PLATFORM_APPLE:
        sprintf( buffer, "\t.globl _%s\n\t.private_extern _%s\n_%s:", func, func, func );
        return buffer;
    case PLATFORM_WINDOWS:
        sprintf( buffer, "\t.globl _%s\n_%s:", func, func );
        return buffer;
    default:
        sprintf( buffer, "\t.globl %s\n\t.hidden %s\n%s:", func, func, func );
        return buffer;
    }
}

const char *get_asm_ptr_keyword(void)
{
    switch(get_ptr_size())
    {
    case 4: return ".long";
    case 8: return ".quad";
    }
    assert(0);
    return NULL;
}

const char *get_asm_string_keyword(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE:
        return ".asciz";
    default:
        return ".string";
    }
}

const char *get_asm_short_keyword(void)
{
    switch (target_platform)
    {
    default:            return ".short";
    }
}

const char *get_asm_rodata_section(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE: return ".const";
    default:             return ".section .rodata";
    }
}

const char *get_asm_string_section(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE: return ".cstring";
    default:             return ".section .rodata";
    }
}
