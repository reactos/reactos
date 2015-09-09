/*
 * CD-ROM Maker
 * by Philip J. Erdelsky
 * pje@acm.org
 * http://alumnus.caltech.edu/~pje/
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CD-ROM Maker
 * FILE:            tools/cdmake/cdmake.c
 * PURPOSE:         CD-ROM Premastering Utility
 * PROGRAMMERS:     Eric Kohl
 *                  Casper S. Hornstrup
 *                  Filip Navara
 *                  Magnus Olsen
 *
 * HISTORY:
 *
 * ElTorito-Support
 * by Eric Kohl
 *
 * Linux port
 * by Casper S. Hornstrup
 * chorns@users.sourceforge.net
 *
 * Joliet support
 * by Filip Navara
 * xnavara@volny.cz
 * Limitations:
 * - No Joliet file name validations
 * - Very bad ISO file name generation
 *
 * Convert long filename to ISO9660 file name by Magnus Olsen
 * magnus@greatlord.com
 */

/* According to his website, this file was released into the public domain by Philip J. Erdelsky */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <io.h>
# include <dos.h>
# ifdef _MSC_VER
#  define R_OK 4
# endif
#else
# if defined(__FreeBSD__) || defined(__APPLE__)
#  include <sys/uio.h>
# else
#  include <sys/io.h>
# endif // __FreeBSD__
# include <errno.h>
# include <sys/types.h>
# include <dirent.h>
# include <unistd.h>
# define TRUE 1
# define FALSE 0
#endif // _WIN32
#include <ctype.h>
#include <time.h>
#include "config.h"
#include "dirhash.h"

// FIXME! FIXME! Do it in a portable way!!
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

// file system parameters

#define MAX_LEVEL               8
#define MAX_NAME_LENGTH         64
#define MAX_CDNAME_LENGTH       8
#define MAX_EXTENSION_LENGTH    10
#define MAX_CDEXTENSION_LENGTH  3
#define SECTOR_SIZE             2048
#define BUFFER_SIZE             (8 * SECTOR_SIZE)

#define HIDDEN_FLAG     1
#define DIRECTORY_FLAG  2


struct cd_image
{
    FILE *file;
    DWORD sector;         // sector to receive next byte
    int offset;           // offset of next byte in sector
    int count;            // number of bytes in buffer
    char filespecs[128];
    BYTE *buffer;
};

typedef struct _boot_validation_header
{
    BYTE  header_id;
    BYTE  platform_id;
} BOOT_VALIDATION_HEADER, *PBOOT_VALIDATION_HEADER;

typedef struct boot_entry
{
    struct boot_entry *next_entry;
    BYTE  boot_id;
    BYTE  boot_emu_type;
    WORD  load_segment;
    BYTE  system_type;
    WORD  sector_count; // boot_image_size
    DWORD load_rba;     // boot_image_sector
    // BYTE  unused[20];
    char bootimage[512];
} BOOT_ENTRY, *PBOOT_ENTRY;

typedef struct boot_header
{
    struct boot_header *next_header;
    BYTE  header_id;
    BYTE  platform_id;
    WORD  num_entries;
    // char  id_string[28];
    PBOOT_ENTRY entry_list;
} BOOT_HEADER, *PBOOT_HEADER;

typedef struct date_and_time
{
    BYTE second;
    BYTE minute;
    BYTE hour;
    BYTE day;
    BYTE month;
    WORD year;
} DATE_AND_TIME, *PDATE_AND_TIME;

typedef struct directory_record
{
    struct directory_record *next_in_directory;
    struct directory_record *next_in_path_table; /* directory record only */
    struct directory_record *next_in_memory;
    struct directory_record *first_record;       /* directory record only */
    struct directory_record *parent;
    BYTE flags;
    char name[MAX_NAME_LENGTH+1];
    char name_on_cd[MAX_CDNAME_LENGTH+1];
    char extension[MAX_EXTENSION_LENGTH+1];
    char extension_on_cd[MAX_CDEXTENSION_LENGTH+1];
    char *joliet_name;
    const char *orig_name;
    DATE_AND_TIME date_and_time;
    DWORD sector;
    DWORD size;
    DWORD joliet_sector;
    DWORD joliet_size;
    unsigned level;                             /* directory record only */
    WORD path_table_index;                      /* directory record only */
} DIR_RECORD, *PDIR_RECORD;

typedef enum directory_record_type
{
    DOT_RECORD,
    DOT_DOT_RECORD,
    SUBDIRECTORY_RECORD,
    FILE_RECORD
} DIR_RECORD_TYPE, *PDIR_RECORD_TYPE;


PDIR_RECORD sort_linked_list(PDIR_RECORD,
                             unsigned, int (*)(PDIR_RECORD, PDIR_RECORD));


static char DIRECTORY_TIMESTAMP[] = "~Y$'KOR$.3K&";

static struct cd_image cd;

char volume_label[32];
DIR_RECORD root;
char source[512];
char *end_source;
enum {QUIET, NORMAL, VERBOSE} verbosity;
BOOL show_progress;
DWORD size_limit;
BOOL accept_punctuation_marks;

DWORD total_sectors;
DWORD path_table_size;
DWORD little_endian_path_table_sector;
DWORD big_endian_path_table_sector;
DWORD number_of_files;
DWORD bytes_in_files;
DWORD unused_bytes_at_ends_of_files;
DWORD number_of_directories;
DWORD bytes_in_directories;

BOOL eltorito;   // TRUE/FALSE: bootable/non-bootable CD-ROM
BOOL multi_boot; // TRUE/FALSE: multi/single-boot CD-ROM
DWORD boot_catalog_sector;
BOOT_VALIDATION_HEADER boot_validation_header;
BOOT_ENTRY default_boot_entry;
PBOOT_HEADER boot_header_list;

BOOL joliet;
DWORD joliet_path_table_size;
DWORD joliet_little_endian_path_table_sector;
DWORD joliet_big_endian_path_table_sector;

struct target_dir_hash specified_files;

/*-----------------------------------------------------------------------------
This function edits a 32-bit unsigned number into a comma-delimited form, such
as 4,294,967,295 for the largest possible number, and returns a pointer to a
static buffer containing the result. It suppresses leading zeros and commas,
but optionally pads the result with blanks at the left so the result is always
exactly 13 characters long (excluding the terminating zero).

CAUTION: A statement containing more than one call on this function, such as
printf("%s, %s", edit_with_commas(a), edit_with_commas(b)), will produce
incorrect results because all calls use the same static bufffer.
-----------------------------------------------------------------------------*/

static char *edit_with_commas(DWORD x, BOOL pad)
{
    static char s[14];
    unsigned i = 13;
    do
    {
        if (i % 4 == 2) s[--i] = ',';
        s[--i] = (char)(x % 10 + '0');
    } while ((x/=10) != 0);
    if (pad)
    {
        while (i > 0) s[--i] = ' ';
    }
    return s + i;
}

/*-----------------------------------------------------------------------------
This function releases all allocated memory blocks.
-----------------------------------------------------------------------------*/

static void release_memory(void)
{
    while (boot_header_list)
    {
        PBOOT_HEADER next_header = boot_header_list->next_header;

        while (boot_header_list->entry_list)
        {
            PBOOT_ENTRY next_entry = boot_header_list->entry_list->next_entry;
            free(boot_header_list->entry_list);
            boot_header_list->entry_list = next_entry;
        }

        free(boot_header_list);
        boot_header_list = next_header;
    }

    while (root.next_in_memory != NULL)
    {
        PDIR_RECORD next = root.next_in_memory->next_in_memory;
        if (joliet)
            free(root.next_in_memory->joliet_name);
        free(root.next_in_memory);
        root.next_in_memory = next;
    }
    if (joliet)
        free(root.joliet_name);

    if (cd.buffer != NULL)
    {
        free(cd.buffer);
        cd.buffer = NULL;
    }
}

/*-----------------------------------------------------------------------------
This function edits and displays an error message and then jumps back to the
error exit point in main().
-----------------------------------------------------------------------------*/

static void error_exit(const char* fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);
    printf("\n");
    if (cd.file != NULL)
        fclose(cd.file);
    release_memory();
    exit(1);
}

/*-----------------------------------------------------------------------------
This function, which is called only on the second pass, and only when the
buffer is not empty, flushes the buffer to the CD-ROM image.
-----------------------------------------------------------------------------*/

static void flush_buffer(void)
{
    if (fwrite(cd.buffer, cd.count, 1, cd.file) < 1)
        error_exit("File write error");
    cd.count = 0;
    if (show_progress)
    {
        printf("\r%s ",
            edit_with_commas((total_sectors - cd.sector) * SECTOR_SIZE, TRUE));
    }
}

/*-----------------------------------------------------------------------------
This function writes a single byte to the CD-ROM image. On the first pass (in
which cd.handle < 0), it does not actually write anything but merely updates
the file pointer as though the byte had been written.
-----------------------------------------------------------------------------*/

static void write_byte(BYTE x)
{
    if (cd.file != NULL)
    {
        cd.buffer[cd.count] = x;
        if (++cd.count == BUFFER_SIZE)
            flush_buffer();
    }
    if (++cd.offset == SECTOR_SIZE)
    {
        cd.sector++;
        cd.offset = 0;
    }
}

/*-----------------------------------------------------------------------------
These functions write a word or double word to the CD-ROM image with the
specified endianity.
-----------------------------------------------------------------------------*/

static void write_little_endian_word(WORD x)
{
    write_byte((BYTE)x);
    write_byte((BYTE)(x >> 8));
}

static void write_big_endian_word(WORD x)
{
    write_byte((BYTE)(x >> 8));
    write_byte((BYTE)x);
}

static void write_both_endian_word(WORD x)
{
    write_little_endian_word(x);
    write_big_endian_word(x);
}

static void write_little_endian_dword(DWORD x)
{
    write_byte((BYTE)x);
    write_byte((BYTE)(x >> 8));
    write_byte((BYTE)(x >> 16));
    write_byte((BYTE)(x >> 24));
}

static void write_big_endian_dword(DWORD x)
{
    write_byte((BYTE)(x >> 24));
    write_byte((BYTE)(x >> 16));
    write_byte((BYTE)(x >> 8));
    write_byte((BYTE)x);
}

static void write_both_endian_dword(DWORD x)
{
    write_little_endian_dword(x);
    write_big_endian_dword(x);
}

/*-----------------------------------------------------------------------------
This function writes enough zeros to fill out the end of a sector, and leaves
the file pointer at the beginning of the next sector. If the file pointer is
already at the beginning of a sector, it writes nothing.
-----------------------------------------------------------------------------*/

static void fill_sector(void)
{
    while (cd.offset != 0)
        write_byte(0);
}

/*-----------------------------------------------------------------------------
This function writes a string to the CD-ROM image. The terminating \0 is not
written.
-----------------------------------------------------------------------------*/

static void write_string(char *s)
{
    while (*s != 0)
        write_byte(*s++);
}

static void write_bytecounted_string(unsigned bytecount, char *s, char padding)
{
    while (*s != 0 && bytecount != 0)
    {
        write_byte(*s++);
        bytecount--;
    }
    while (bytecount != 0)
    {
        write_byte(padding);
        bytecount--;
    }
}

/*-----------------------------------------------------------------------------
This function writes a ansi string as a big endian unicode string to the CD-ROM
image. The terminating \0 is not written.
-----------------------------------------------------------------------------*/

static void write_string_as_big_endian_unicode(char *s)
{
    while (*s != 0)
    {
        write_big_endian_word(*s++);
    }
}

static void write_bytecounted_string_as_big_endian_unicode(unsigned bytecount, char *s, char padding)
{
    unsigned wordcount = bytecount / 2;

    while (*s != 0 && wordcount != 0)
    {
        write_big_endian_word(*s++);
        wordcount--;
    }
    while (wordcount != 0)
    {
        write_big_endian_word(padding);
        wordcount--;
    }

    if (bytecount % 2 != 0)
        write_byte(padding);
}

/*-----------------------------------------------------------------------------
This function writes a block of identical bytes to the CD-ROM image.
-----------------------------------------------------------------------------*/

static void write_block(unsigned count, BYTE value)
{
    while (count != 0)
    {
        write_byte(value);
        count--;
    }
}

/*-----------------------------------------------------------------------------
This function writes a block of identical big endian words to the CD-ROM image.
-----------------------------------------------------------------------------*/

static void write_word_block(unsigned count, WORD value)
{
    while (count != 0)
    {
        write_big_endian_word(value);
        count--;
    }
}

/*-----------------------------------------------------------------------------
This function writes a directory record to the CD_ROM image.
-----------------------------------------------------------------------------*/

static void
write_directory_record(PDIR_RECORD d,
                       DIR_RECORD_TYPE DirType,
                       BOOL joliet)
{
    unsigned identifier_size;
    unsigned record_size;

    if (joliet)
    {
        if (DirType == DOT_RECORD || DirType == DOT_DOT_RECORD)
            identifier_size = 1;
        else
            identifier_size = strlen(d->joliet_name) * 2;
    }
    else
    {
        switch (DirType)
        {
            case DOT_RECORD:
            case DOT_DOT_RECORD:
                identifier_size = 1;
                break;
            case SUBDIRECTORY_RECORD:
                /*printf("Subdir: %s\n", d->name_on_cd);*/
                identifier_size = strlen(d->name_on_cd);
                break;
            case FILE_RECORD:
                /*printf("File: %s.%s -> %s.%s\n", d->name, d->extension, d->name_on_cd, d->extension_on_cd);*/
                identifier_size = strlen(d->name_on_cd) + 2;
                if (d->extension_on_cd[0] != 0)
                    identifier_size += 1 + strlen(d->extension_on_cd);
                break;
            default:
                identifier_size = 1;
                break;
        }
    }
    record_size = 33 + identifier_size;
    if ((identifier_size & 1) == 0)
        record_size++;
    if (cd.offset + record_size > SECTOR_SIZE)
        fill_sector();
    write_byte((BYTE)record_size);
    write_byte(0); // number of sectors in extended attribute record
    if (joliet)
    {
        write_both_endian_dword(d->joliet_sector);
        write_both_endian_dword(d->joliet_size);
    }
    else
    {
        write_both_endian_dword(d->sector);
        write_both_endian_dword(d->size);
    }
    write_byte((BYTE)(d->date_and_time.year - 1900));
    write_byte(d->date_and_time.month);
    write_byte(d->date_and_time.day);
    write_byte(d->date_and_time.hour);
    write_byte(d->date_and_time.minute);
    write_byte(d->date_and_time.second);
    write_byte(0);    // GMT offset
    write_byte(d->flags);
    write_byte(0);    // file unit size for an interleaved file
    write_byte(0);    // interleave gap size for an interleaved file
    write_both_endian_word(1); // volume sequence number
    write_byte((BYTE)identifier_size);
    switch (DirType)
    {
        case DOT_RECORD:
            write_byte(0);
            break;
        case DOT_DOT_RECORD:
            write_byte(1);
            break;
        case SUBDIRECTORY_RECORD:
            if (joliet)
                write_string_as_big_endian_unicode(d->joliet_name);
            else
                write_string(d->name_on_cd);
            break;
        case FILE_RECORD:
            if (joliet)
            {
                write_string_as_big_endian_unicode(d->joliet_name);
            }
            else
            {
                write_string(d->name_on_cd);
                if (d->extension_on_cd[0] != 0)
                {
                    write_byte('.');
                    write_string(d->extension_on_cd);
                }
                write_string(";1");
            }
            break;
    }
    if ((identifier_size & 1) == 0)
        write_byte(0);
}

/*-----------------------------------------------------------------------------
This function converts the date and time words from an ffblk structure and
puts them into a date_and_time structure.
-----------------------------------------------------------------------------*/

static void convert_date_and_time(PDATE_AND_TIME dt, time_t *time)
{
    struct tm *timedef;
    timedef = gmtime(time);

    dt->second = timedef->tm_sec;
    dt->minute = timedef->tm_min;
    dt->hour = timedef->tm_hour;
    dt->day = timedef->tm_mday;
    dt->month = timedef->tm_mon + 1;
    dt->year = timedef->tm_year + 1900;
}

/*-----------------------------------------------------------------------------
This function checks the specified character, if necessary, and
generates an error if it is a punctuation mark other than an underscore.
It also converts small letters to capital letters and returns the
result.
-----------------------------------------------------------------------------*/

static int check_for_punctuation(int c, const char *name)
{
    c = toupper(c & 0xFF);
    if (!accept_punctuation_marks && !isalnum(c) && c != '_')
        error_exit("Punctuation mark in %s", name);
    return c;
}

/*-----------------------------------------------------------------------------
This function checks to see if there's a cdname conflict.
-----------------------------------------------------------------------------*/

#if defined(_WIN32) && !defined(strcasecmp)
#define strcasecmp stricmp
#endif//_WIN32

static BOOL cdname_exists(PDIR_RECORD d)
{
    PDIR_RECORD p = d->parent->first_record;
    while (p)
    {
        if ( p != d
            && !strcasecmp(p->name_on_cd, d->name_on_cd)
            && !strcasecmp(p->extension_on_cd, d->extension_on_cd) )
            return TRUE;
        p = p->next_in_directory;
    }
    return FALSE;
}

static void parse_filename_into_dirrecord(const char* filename, PDIR_RECORD d, BOOL dir)
{
    const char *s = filename;
    char *t = d->name_on_cd;
    char *n = d->name;
    int joliet_length;
    int filename_counter;
    filename_counter = 1;
    while (*s != 0)
    {
        if (*s == '.')
        {
            s++;
            break;
        }

        if ((size_t)(t-d->name_on_cd) < sizeof(d->name_on_cd)-1)
            *t++ = check_for_punctuation(*s, filename);
        else if (!joliet)
            error_exit("'%s' is not ISO-9660, aborting...", filename);

        if ((size_t)(n-d->name) < sizeof(d->name)-1)
            *n++ = *s;
        else if (!joliet)
            error_exit("'%s' is not ISO-9660, aborting...", filename);
        s++;
    }
    if (strlen(s) > MAX_EXTENSION_LENGTH)
    {
        error_exit("'%s' has too long extension, aborting...", filename);
    }
    *t = 0;
    strcpy(d->extension, s);
    t = d->extension_on_cd;
    while (*s != 0)
    {
        if ((size_t)(t-d->extension_on_cd) < sizeof(d->extension_on_cd)-1)
            *t++ = check_for_punctuation(*s, filename);
        else if (!joliet)
            error_exit("'%s' is not ISO-9660, aborting...", filename);
        s++;
    }
    *t = 0;
    *n = 0;

    if (dir)
    {
        if (d->extension[0] != 0)
        {
            if (!joliet)
                error_exit("Directory with extension %s", filename);
        }
        d->flags = DIRECTORY_FLAG;
    } else
    {
        d->flags = 0;
    }

    filename_counter = 1;
    while (cdname_exists(d))
    {
        // the file name must be least 8 char long
        if (strlen(d->name_on_cd)<8)
            error_exit("'%s' is a duplicate file name, aborting...", filename);

        if ((d->name_on_cd[8] == '.') && (strlen(d->name_on_cd) < 13))
            error_exit("'%s' is a duplicate file name, aborting...", filename);

        // max 255 times for equal short filename
        if (filename_counter>255) error_exit("'%s' is a duplicate file name, aborting...", filename);
        d->name_on_cd[8] = '~';
        memset(&d->name_on_cd[9],0,5);
        sprintf(&d->name_on_cd[9],"%d",filename_counter);
        filename_counter++;
    }

    if (joliet)
    {
        joliet_length = strlen(filename);
        if (joliet_length > 64)
            error_exit("'%s' is not Joliet, aborting...", filename);
        d->joliet_name = malloc(joliet_length + 1);
        if (d->joliet_name == NULL)
            error_exit("Insufficient memory");
        strcpy(d->joliet_name, filename);
    }
}

/*-----------------------------------------------------------------------------
This function creates a new directory record with the information from the
specified ffblk. It links it into the beginning of the directory list
for the specified parent and returns a pointer to the new record.
-----------------------------------------------------------------------------*/

#if _WIN32

/* Win32 version */
PDIR_RECORD
new_directory_record(struct _finddata_t *f,
                     PDIR_RECORD parent)
{
    PDIR_RECORD d;

    d = calloc(1, sizeof(*d));
    if (d == NULL)
        error_exit("Insufficient memory");
    d->next_in_memory = root.next_in_memory;
    root.next_in_memory = d;

    /* I need the parent set before calling parse_filename_into_dirrecord(),
    because that functions checks for duplicate file names*/
    d->parent = parent;
    parse_filename_into_dirrecord(f->name, d, f->attrib & _A_SUBDIR);

    convert_date_and_time(&d->date_and_time, &f->time_write);
    d->flags |= f->attrib & _A_HIDDEN ? HIDDEN_FLAG : 0;
    d->size = d->joliet_size = f->size;
    d->next_in_directory = parent->first_record;
    parent->first_record = d;
    return d;
}

#else

/* Linux version */
PDIR_RECORD
new_directory_record(struct dirent *entry,
                     struct stat *stbuf,
                     PDIR_RECORD parent)
{
    PDIR_RECORD d;

    d = calloc(1, sizeof(*d));
    if (d == NULL)
        error_exit("Insufficient memory");
    d->next_in_memory = root.next_in_memory;
    root.next_in_memory = d;

    /* I need the parent set before calling parse_filename_into_dirrecord(),
    because that functions checks for duplicate file names*/
    d->parent = parent;
#ifdef HAVE_D_TYPE
    parse_filename_into_dirrecord(entry->d_name, d, entry->d_type == DT_DIR);
#else
    parse_filename_into_dirrecord(entry->d_name, d, S_ISDIR(stbuf->st_mode));
#endif

    convert_date_and_time(&d->date_and_time, &stbuf->st_mtime);
    d->flags |= entry->d_name[0] == '.' ? HIDDEN_FLAG : 0;
    d->size = d->joliet_size = stbuf->st_size;
    d->next_in_directory = parent->first_record;
    parent->first_record = d;
    return d;
}

#endif

/*-----------------------------------------------------------------------------
This function compares two directory records according to the ISO9660 rules
for directory sorting and returns a negative value if p is before q, or a
positive value if p is after q.
-----------------------------------------------------------------------------*/

static int compare_directory_order(PDIR_RECORD p, PDIR_RECORD q)
{
    int n = strcmp(p->name_on_cd, q->name_on_cd);
    if (n == 0)
        n = strcmp(p->extension_on_cd, q->extension_on_cd);
    return n;
}

/*-----------------------------------------------------------------------------
This function compares two directory records (which must represent
directories) according to the ISO9660 rules for path table sorting and returns
a negative value if p is before q, or a positive vlaue if p is after q.
-----------------------------------------------------------------------------*/

static int compare_path_table_order(PDIR_RECORD p, PDIR_RECORD q)
{
    int n = p->level - q->level;
    if (p == q)
        return 0;
    if (n == 0)
    {
        n = compare_path_table_order(p->parent, q->parent);
        if (n == 0)
            n = compare_directory_order(p, q);
    }
    return n;
}

/*-----------------------------------------------------------------------------
This function appends the specified string to the buffer source[].
-----------------------------------------------------------------------------*/

static void append_string_to_source(char *s)
{
    while (*s != 0)
        *end_source++ = *s++;
}

/*-----------------------------------------------------------------------------
This function scans all files from the current source[] (which must end in \,
and represents a directory already in the database as d),
and puts the appropriate directory records into the database in memory, with
the specified root. It calls itself recursively to scan all subdirectories.
-----------------------------------------------------------------------------*/

#ifdef _WIN32

static void
make_directory_records(PDIR_RECORD d)
{
    PDIR_RECORD new_d;
    struct _finddata_t f;
    char *old_end_source;
    int findhandle;

    d->first_record = NULL;
    strcpy(end_source, "*.*");

    findhandle = _findfirst(source, &f);
    if (findhandle != 0)
    {
        do
        {
            if ((f.attrib & (_A_HIDDEN | _A_SUBDIR)) == 0 && f.name[0] != '.')
            {
                if (strcmp(f.name, DIRECTORY_TIMESTAMP) == 0)
                {
                    convert_date_and_time(&d->date_and_time, &f.time_write);
                }
                else
                {
                    if (verbosity == VERBOSE)
                    {
                        old_end_source = end_source;
                        strcpy(end_source, f.name);
                        printf("%d: file %s\n", d->level, source);
                        end_source = old_end_source;
                    }
                    (void) new_directory_record(&f, d);
                }
            }
        }
        while (_findnext(findhandle, &f) == 0);

        _findclose(findhandle);
    }

    strcpy(end_source, "*.*");
    findhandle = _findfirst(source, &f);
    if (findhandle)
    {
        do
        {
            if (f.attrib & _A_SUBDIR && f.name[0] != '.')
            {
                old_end_source = end_source;
                append_string_to_source(f.name);
                *end_source++ = DIR_SEPARATOR_CHAR;
                if (verbosity == VERBOSE)
                {
                    *end_source = 0;
                    printf("%d: directory %s\n", d->level + 1, source);
                }
                if (d->level < MAX_LEVEL)
                {
                    new_d = new_directory_record(&f, d);
                    new_d->next_in_path_table = root.next_in_path_table;
                    root.next_in_path_table = new_d;
                    new_d->level = d->level + 1;
                    make_directory_records(new_d);
                }
                else
                {
                    error_exit("Directory is nested too deep");
                }
                end_source = old_end_source;
            }
        }
        while (_findnext(findhandle, &f) == 0);

      _findclose(findhandle);
    }

    // sort directory
    d->first_record = sort_linked_list(d->first_record, 0, compare_directory_order);
}

#else

/* Linux version */
static void
make_directory_records(PDIR_RECORD d)
{
    PDIR_RECORD new_d;
    DIR *dirp;
    struct dirent *entry;
    char *old_end_source;
    struct stat stbuf;
    char buf[MAX_PATH];

    d->first_record = NULL;

#ifdef HAVE_D_TYPE
    dirp = opendir(source);
    if (dirp != NULL)
    {
        while ((entry = readdir(dirp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue; // skip self and parent

            if (entry->d_type == DT_REG) // normal file
            {
                // Check for an absolute path
                if (source[0] == DIR_SEPARATOR_CHAR)
                {
                    strcpy(buf, source);
                    strcat(buf, DIR_SEPARATOR_STRING);
                    strcat(buf, entry->d_name);
                }
                else
                {
                    if (!getcwd(buf, sizeof(buf)))
                        error_exit("Cannot get CWD: %s\n", strerror(errno));
                    strcat(buf, DIR_SEPARATOR_STRING);
                    strcat(buf, source);
                    strcat(buf, entry->d_name);
                }

                if (stat(buf, &stbuf) == -1)
                {
                    error_exit("Cannot access '%s' (%s)\n", buf, strerror(errno));
                    return;
                }

                if (strcmp(entry->d_name, DIRECTORY_TIMESTAMP) == 0)
                {
                    convert_date_and_time(&d->date_and_time, &stbuf.st_ctime);
                }
                else
                {
                    if (verbosity == VERBOSE)
                    {
                        printf("%d: file %s\n", d->level, buf);
                    }
                    (void) new_directory_record(entry, &stbuf, d);
                }
            }
        }
        closedir(dirp);
    }
    else
    {
        error_exit("Cannot open %s\n", source);
        return;
    }

    dirp = opendir(source);
    if (dirp != NULL)
    {
        while ((entry = readdir(dirp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue; // skip self and parent

            if (entry->d_type == DT_DIR) // directory
            {
                old_end_source = end_source;
                append_string_to_source(entry->d_name);
                *end_source++ = DIR_SEPARATOR_CHAR;
                *end_source = 0;
                if (verbosity == VERBOSE)
                {
                    printf("%d: directory %s\n", d->level + 1, source);
                }
                if (d->level < MAX_LEVEL)
                {
                    // Check for an absolute path
                    if (source[0] == DIR_SEPARATOR_CHAR)
                    {
                        strcpy(buf, source);
                    }
                    else
                    {
                        if (!getcwd(buf, sizeof(buf)))
                            error_exit("Cannot get CWD: %s\n", strerror(errno));
                        strcat(buf, DIR_SEPARATOR_STRING);
                        strcat(buf, source);
                    }

                    if (stat(buf, &stbuf) == -1)
                    {
                        error_exit("Cannot access '%s' (%s)\n", buf, strerror(errno));
                        return;
                    }
                    new_d = new_directory_record(entry, &stbuf, d);
                    new_d->next_in_path_table = root.next_in_path_table;
                    root.next_in_path_table = new_d;
                    new_d->level = d->level + 1;
                    make_directory_records(new_d);
                }
                else
                {
                    error_exit("Directory is nested too deep");
                }
                end_source = old_end_source;
                *end_source = 0;
            }
        }
        closedir(dirp);
    }
    else
    {
        error_exit("Cannot open %s\n", source);
        return;
    }

#else

    dirp = opendir(source);
    if (dirp != NULL)
    {
        while ((entry = readdir(dirp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue; // skip self and parent

            // Check for an absolute path
            if (source[0] == DIR_SEPARATOR_CHAR)
            {
                strcpy(buf, source);
                strcat(buf, DIR_SEPARATOR_STRING);
                strcat(buf, entry->d_name);
            }
            else
            {
                if (!getcwd(buf, sizeof(buf)))
                    error_exit("Cannot get CWD: %s\n", strerror(errno));
                strcat(buf, DIR_SEPARATOR_STRING);
                strcat(buf, source);
                strcat(buf, entry->d_name);
            }

            if (stat(buf, &stbuf) == -1)
            {
                error_exit("Cannot access '%s' (%s)\n", buf, strerror(errno));
                return;
            }

            if (S_ISDIR(stbuf.st_mode))
            {
                old_end_source = end_source;
                append_string_to_source(entry->d_name);
                *end_source++ = DIR_SEPARATOR_CHAR;
                *end_source = 0;
                if (verbosity == VERBOSE)
                {
                    printf("%d: directory %s\n", d->level + 1, source);
                }

                if (d->level < MAX_LEVEL)
                {
                    new_d = new_directory_record(entry, &stbuf, d);
                    new_d->next_in_path_table = root.next_in_path_table;
                    root.next_in_path_table = new_d;
                    new_d->level = d->level + 1;
                    make_directory_records(new_d);
                }
                else
                {
                    error_exit("Directory is nested too deep");
                }

                end_source = old_end_source;
                *end_source = 0;
            }
            else if (S_ISREG(stbuf.st_mode))
            {
                if (strcmp(entry->d_name, DIRECTORY_TIMESTAMP) == 0)
                {
                    convert_date_and_time(&d->date_and_time, &stbuf.st_ctime);
                }
                else
                {
                    if (verbosity == VERBOSE)
                    {
                        printf("%d: file %s\n", d->level, buf);
                    }
                    (void) new_directory_record(entry, &stbuf, d);
                }
            }
        }
        closedir(dirp);
    }
    else
    {
        error_exit("Cannot open %s\n", source);
        return;
    }

#endif

    // sort directory
    d->first_record = sort_linked_list(d->first_record, 0, compare_directory_order);
}

#endif

static PDIR_RECORD
new_empty_dirrecord(PDIR_RECORD d, BOOL directory)
{
    PDIR_RECORD new_d;
    new_d = calloc(1, sizeof(*new_d));
    new_d->parent = d;
    new_d->level = d->level + 1;
    new_d->next_in_directory = d->first_record;
    d->first_record = new_d;
    new_d->next_in_memory = root.next_in_memory;
    root.next_in_memory = new_d;
    new_d->date_and_time = d->date_and_time;
    if (directory)
    {
        new_d->flags |= DIRECTORY_FLAG;
        new_d->next_in_path_table = root.next_in_path_table;
        root.next_in_path_table = new_d;
    }
    return new_d;
}

#if _WIN32
static BOOL
get_cd_file_time(HANDLE handle, PDATE_AND_TIME cd_time_info)
{
    FILETIME file_time;
    SYSTEMTIME sys_time;

    if (!GetFileTime(handle, NULL, NULL, &file_time))
        return FALSE;

    FileTimeToSystemTime(&file_time, &sys_time);
    memset(cd_time_info, 0, sizeof(*cd_time_info));

    cd_time_info->year = sys_time.wYear;
    cd_time_info->month = sys_time.wMonth;
    cd_time_info->day = sys_time.wDay;
    cd_time_info->hour = sys_time.wHour;
    cd_time_info->minute = sys_time.wMinute;
    cd_time_info->second = sys_time.wSecond;

    return TRUE;
}
#endif

static void
scan_specified_files(PDIR_RECORD d, struct target_dir_entry *dir)
{
    PDIR_RECORD new_d;
#if _WIN32
    HANDLE open_file;
    LARGE_INTEGER file_size;
#else
    struct stat stbuf;
#endif
    struct target_file *file;
    struct target_dir_entry *child;

    d->first_record = NULL;

    for (file = dir->head; file; file = file->next)
    {
        if (strcmp(file->target_name, DIRECTORY_TIMESTAMP) == 0)
        {
#if _WIN32
            if ((open_file = CreateFileA(file->source_name,
                                         GENERIC_READ,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL)) == INVALID_HANDLE_VALUE)
            {
                error_exit("Cannot open timestamp file %s\n", file->source_name);
            }

            if (!get_cd_file_time(open_file, &d->date_and_time))
            {
                error_exit("Cannot stat timestamp file %s\n", file->source_name);
            }
            CloseHandle(open_file);
#else
            if (stat(file->target_name, &stbuf) == -1)
            {
                error_exit("Cannot stat timestamp file %s\n", file->source_name);
            }
            convert_date_and_time(&d->date_and_time, &stbuf.st_ctime);
#endif
        }
        else
        {
            if (verbosity == VERBOSE)
            {
                printf("%d: file %s (from %s)\n",
                       d->level,
                       file->target_name,
                       file->source_name);
            }
            new_d = new_empty_dirrecord(d, FALSE);
            parse_filename_into_dirrecord(file->target_name, new_d, FALSE);
#if _WIN32
            if ((open_file = CreateFileA(file->source_name,
                                         GENERIC_READ,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL)) == INVALID_HANDLE_VALUE)
            {
                error_exit("Cannot open file %s\n", file->source_name);
            }
            if (!get_cd_file_time(open_file, &new_d->date_and_time))
            {
                error_exit("Cannot stat file %s\n", file->source_name);
            }
            if (!GetFileSizeEx(open_file, &file_size))
            {
                error_exit("Cannot get file size of %s\n", file->source_name);
            }
            new_d->size = new_d->joliet_size = file_size.QuadPart;
            new_d->orig_name = file->source_name;
            CloseHandle(open_file);
#else
            if (stat(file->source_name, &stbuf) == -1)
            {
                error_exit("Cannot find '%s' (target %s)\n",
                           file->source_name,
                           file->target_name);
            }
            convert_date_and_time(&new_d->date_and_time, &stbuf.st_mtime);
            new_d->size = new_d->joliet_size = stbuf.st_size;
            new_d->orig_name = file->source_name;
#endif
        }
    }

    for (child = dir->child; child; child = child->next)
    {
        if (verbosity == VERBOSE)
        {
            printf("%d: directory %s\n", d->level, child->case_name);
        }
        new_d = new_empty_dirrecord(d, TRUE);
        parse_filename_into_dirrecord(child->case_name, new_d, TRUE);
        scan_specified_files(new_d, child);
    }

    /* sort directory */
    d->first_record = sort_linked_list(d->first_record,
                                       0,
                                       compare_directory_order);
    source[0] = 0;
    end_source = source;
}

/*-----------------------------------------------------------------------------
This function loads the file specifications for the file or directory
corresponding to the specified directory record into the source[] buffer. It
is recursive.
-----------------------------------------------------------------------------*/

static void get_file_specifications(PDIR_RECORD d)
{
    if (d != &root)
    {
        get_file_specifications(d->parent);
        if (d->joliet_name == NULL)
            append_string_to_source(d->name);
        else
            append_string_to_source(d->joliet_name);

        if (((d->flags & DIRECTORY_FLAG) == 0 || joliet) && d->extension[0] != 0)
        {
            if (d->joliet_name == NULL)
            {
                *end_source++ = '.';
                append_string_to_source(d->extension);
            }
        }
        if (d->flags & DIRECTORY_FLAG)
            *end_source++ = DIR_SEPARATOR_CHAR;
    }
}

static void get_time_string(char *str)
{
    sprintf(str, "%04d%02d%02d%02d%02d%02d00",
            root.date_and_time.year,
            root.date_and_time.month,
            root.date_and_time.day,
            root.date_and_time.hour,
            root.date_and_time.minute,
            root.date_and_time.second);
}

static BOOL write_from_file(FILE *file, DWORD size)
{
    if (cd.file != NULL)
    {
        int n;

        fseek(file, 0, SEEK_SET);
        while (size > 0)
        {
            n = BUFFER_SIZE - cd.count;
            if ((DWORD)n > size)
                n = size;

            if (fread(cd.buffer + cd.count, n, 1, file) < 1)
                return FALSE;

            cd.count += n;
            if (cd.count == BUFFER_SIZE)
                flush_buffer();
            cd.sector += n / SECTOR_SIZE;
            cd.offset += n % SECTOR_SIZE;
            size -= n;
        }
    }
    else
    {
        cd.sector += size / SECTOR_SIZE;
        cd.offset += size % SECTOR_SIZE;
    }

    return TRUE;
}

static void pass(void)
{
    PDIR_RECORD d;
    PDIR_RECORD q;
    unsigned int index;
    unsigned int name_length;
    DWORD size;
    DWORD number_of_sectors;
    char *old_end_source;
    FILE *file;
    char timestring[17];

    get_time_string(timestring);

    // first 16 sectors are zeros
    write_block(16 * SECTOR_SIZE, 0);


    // Primary Volume Descriptor

    write_string("\1CD001\1");
    write_byte(0);
    write_bytecounted_string(32, "", ' ');           // system identifier
    write_bytecounted_string(32, volume_label, ' '); // volume label

    write_block(8, 0);
    write_both_endian_dword(total_sectors);
    write_block(32, 0);
    write_both_endian_word(1); // volume set size
    write_both_endian_word(1); // volume sequence number
    write_both_endian_word(2048); // sector size
    write_both_endian_dword(path_table_size);
    write_little_endian_dword(little_endian_path_table_sector);
    write_little_endian_dword(0);  // second little endian path table
    write_big_endian_dword(big_endian_path_table_sector);
    write_big_endian_dword(0);  // second big endian path table
    write_directory_record(&root, DOT_RECORD, FALSE);

    write_bytecounted_string(128, volume_label, ' '); // volume set identifier
    write_bytecounted_string(128, PUBLISHER_ID, ' '); // publisher identifier
    write_bytecounted_string(128, DATA_PREP_ID, ' '); // data preparer identifier
    write_bytecounted_string(128, APP_ID, ' ');       // application identifier

    write_bytecounted_string(37, "", ' '); // copyright file identifier
    write_bytecounted_string(37, "", ' '); // abstract file identifier
    write_bytecounted_string(37, "", ' '); // bibliographic file identifier

    write_string(timestring);  // volume creation
    write_byte(0);
    write_string(timestring);  // most recent modification
    write_byte(0);
    write_string("0000000000000000");  // volume expires
    write_byte(0);
    write_string("0000000000000000");  // volume is effective
    write_byte(0);
    write_byte(1);
    write_byte(0);
    fill_sector();


    // Boot Volume Descriptor
    if (eltorito)
    {
        write_byte(0);  // Boot record ID
        write_string("CD001\1");
        write_bytecounted_string(32, "EL TORITO SPECIFICATION", 0); // El-Torito identifier
        write_block(32, 0);  // unused
        write_little_endian_dword(boot_catalog_sector);  // pointer to boot catalog
        fill_sector();
    }

    // Supplementary Volume Descriptor
    if (joliet)
    {
        write_string("\2CD001\1");
        write_byte(0);
        write_bytecounted_string_as_big_endian_unicode(32, "", ' ');           // system identifier
        write_bytecounted_string_as_big_endian_unicode(32, volume_label, ' '); // volume label

        write_block(8, 0);
        write_both_endian_dword(total_sectors);
        write_string("%/E");
        write_block(29, 0);
        write_both_endian_word(1); // volume set size
        write_both_endian_word(1); // volume sequence number
        write_both_endian_word(2048); // sector size
        write_both_endian_dword(joliet_path_table_size);
        write_little_endian_dword(joliet_little_endian_path_table_sector);
        write_little_endian_dword(0);  // second little endian path table
        write_big_endian_dword(joliet_big_endian_path_table_sector);
        write_big_endian_dword(0);  // second big endian path table
        write_directory_record(&root, DOT_RECORD, TRUE);

        write_bytecounted_string_as_big_endian_unicode(128, volume_label, ' '); // volume set identifier
        write_bytecounted_string_as_big_endian_unicode(128, PUBLISHER_ID, ' '); // publisher identifier
        write_bytecounted_string_as_big_endian_unicode(128, DATA_PREP_ID, ' '); // data preparer identifier
        write_bytecounted_string_as_big_endian_unicode(128, APP_ID, ' ');       // application identifier

        write_bytecounted_string_as_big_endian_unicode(37, "", ' '); // copyright file identifier
        write_bytecounted_string_as_big_endian_unicode(37, "", ' '); // abstract file identifier
        write_bytecounted_string_as_big_endian_unicode(37, "", ' '); // bibliographic file identifier

        write_string(timestring);  // volume creation
        write_byte(0);
        write_string(timestring);  // most recent modification
        write_byte(0);
        write_string("0000000000000000");  // volume expires
        write_byte(0);
        write_string("0000000000000000");  // volume is effective
        write_byte(0);
        write_byte(1);
        write_byte(0);
        fill_sector();
    }

    // Volume Descriptor Set Terminator
    write_string("\377CD001\1");
    fill_sector();

    // Boot Catalog and Images
    if (eltorito)
    {
        PBOOT_HEADER header;
        PBOOT_ENTRY  entry;

        // Boot Catalog

        boot_catalog_sector = cd.sector;

        // Validation entry header
        write_byte(boot_validation_header.header_id);
        write_byte(boot_validation_header.platform_id);
        write_little_endian_word(0);  // reserved
        write_bytecounted_string(24, MANUFACTURER_ID, 0); // Manufacturer identifier
        write_little_endian_word(0x62E);  // checksum // FIXME: This is hardcoded!!
        write_little_endian_word(0xAA55);  // signature

        // Default entry
        write_byte(default_boot_entry.boot_id);
        write_byte(default_boot_entry.boot_emu_type);
        write_little_endian_word(default_boot_entry.load_segment);
        write_byte(0);  // partition type
        write_byte(0);  // unused
        write_little_endian_word(default_boot_entry.sector_count);
        write_little_endian_dword(default_boot_entry.load_rba);
        write_block(20, 0); // unused

        // Loop through each boot header
        header = boot_header_list;
        while (header)
        {
            write_byte(header->header_id);
            write_byte(header->platform_id);
            write_little_endian_word(header->num_entries);
            write_block(28, 0); // Identifier string (unused)

            // Loop through each boot entry
            entry = header->entry_list;
            while (entry)
            {
                write_byte(entry->boot_id);
                write_byte(entry->boot_emu_type);
                write_little_endian_word(entry->load_segment);
                write_byte(0);  // partition type
                write_byte(0);  // unused
                write_little_endian_word(entry->sector_count);
                write_little_endian_dword(entry->load_rba);
                write_block(20, 0); // Selection criteria (unused)

                entry = entry->next_entry;
            }

            header = header->next_header;
        }

        fill_sector();


        // Boot Images

        default_boot_entry.load_rba = cd.sector;

        file = fopen(default_boot_entry.bootimage, "rb");
        if (file == NULL)
            error_exit("Cannot open %s\n", default_boot_entry.bootimage);
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        if (size == 0 || (size % 2048))
        {
            fclose(file);
            error_exit("Invalid boot image size (%lu bytes)\n", size);
        }
        // Sector count in 512 byte sectors and rounded up
        default_boot_entry.sector_count = (size + 511) / 512;
        if (!write_from_file(file, size))
        {
            fclose(file);
            error_exit("Read error in file %s\n", default_boot_entry.bootimage);
        }
        fclose(file);

        // Loop through each boot header
        header = boot_header_list;
        while (header)
        {
            // Loop through each boot entry
            entry = header->entry_list;
            while (entry)
            {
                entry->load_rba = cd.sector;

                file = fopen(entry->bootimage, "rb");
                if (file == NULL)
                    error_exit("Cannot open %s\n", entry->bootimage);
                fseek(file, 0, SEEK_END);
                size = ftell(file);
                if (size == 0 || (size % 2048))
                {
                    fclose(file);
                    error_exit("Invalid boot image size (%lu bytes)\n", size);
                }
                // Sector count in 512 byte sectors and rounded up
                entry->sector_count = (size + 511) / 512;
                if (!write_from_file(file, size))
                {
                    fclose(file);
                    error_exit("Read error in file %s\n", entry->bootimage);
                }
                fclose(file);

                entry = entry->next_entry;
            }

            header = header->next_header;
        }

//      fill_sector();
    }

    // Little Endian Path Table
    little_endian_path_table_sector = cd.sector;
    write_byte(1);
    write_byte(0);  // number of sectors in extended attribute record
    write_little_endian_dword(root.sector);
    write_little_endian_word(1);
    write_byte(0);
    write_byte(0);

    index = 1;
    root.path_table_index = 1;
    for (d = root.next_in_path_table; d != NULL; d = d->next_in_path_table)
    {
        name_length = strlen(d->name_on_cd);
        write_byte((BYTE)name_length);
        write_byte(0);  // number of sectors in extended attribute record
        write_little_endian_dword(d->sector);
        write_little_endian_word(d->parent->path_table_index);
        write_string(d->name_on_cd);
        if (name_length & 1)
            write_byte(0);
        d->path_table_index = ++index;
    }

    path_table_size = (cd.sector - little_endian_path_table_sector) *
                       SECTOR_SIZE + cd.offset;
    fill_sector();

    // Big Endian Path Table

    big_endian_path_table_sector = cd.sector;
    write_byte(1);
    write_byte(0);  // number of sectors in extended attribute record
    write_big_endian_dword(root.sector);
    write_big_endian_word(1);
    write_byte(0);
    write_byte(0);

    for (d = root.next_in_path_table; d != NULL; d = d->next_in_path_table)
    {
        name_length = strlen(d->name_on_cd);
        write_byte((BYTE)name_length);
        write_byte(0);  // number of sectors in extended attribute record
        write_big_endian_dword(d->sector);
        write_big_endian_word(d->parent->path_table_index);
        write_string(d->name_on_cd);
        if (name_length & 1)
            write_byte(0);
    }
    fill_sector();

    if (joliet)
    {
        // Little Endian Path Table

        joliet_little_endian_path_table_sector = cd.sector;
        write_byte(1);
        write_byte(0);  // number of sectors in extended attribute record
        write_little_endian_dword(root.joliet_sector);
        write_little_endian_word(1);
        write_byte(0);
        write_byte(0);

        for (d = root.next_in_path_table; d != NULL; d = d->next_in_path_table)
        {
            name_length = strlen(d->joliet_name) * 2;
            write_byte((BYTE)name_length);
            write_byte(0);  // number of sectors in extended attribute record
            write_little_endian_dword(d->joliet_sector);
            write_little_endian_word(d->parent->path_table_index);
            write_string_as_big_endian_unicode(d->joliet_name);
        }

        joliet_path_table_size = (cd.sector - joliet_little_endian_path_table_sector) *
                                  SECTOR_SIZE + cd.offset;
        fill_sector();

        // Big Endian Path Table

        joliet_big_endian_path_table_sector = cd.sector;
        write_byte(1);
        write_byte(0);  // number of sectors in extended attribute record
        write_big_endian_dword(root.joliet_sector);
        write_big_endian_word(1);
        write_byte(0);
        write_byte(0);

        for (d = root.next_in_path_table; d != NULL; d = d->next_in_path_table)
        {
            name_length = strlen(d->joliet_name) * 2;
            write_byte((BYTE)name_length);
            write_byte(0);  // number of sectors in extended attribute record
            write_big_endian_dword(d->joliet_sector);
            write_big_endian_word(d->parent->path_table_index);
            write_string_as_big_endian_unicode(d->joliet_name);
        }
        fill_sector();
    }

    // Directories and files
    for (d = &root; d != NULL; d = d->next_in_path_table)
    {
        // write directory
        d->sector = cd.sector;
        write_directory_record(d, DOT_RECORD, FALSE);
        write_directory_record(d == &root ? d : d->parent, DOT_DOT_RECORD, FALSE);
        for (q = d->first_record; q != NULL; q = q->next_in_directory)
        {
            write_directory_record(q,
                                   q->flags & DIRECTORY_FLAG ? SUBDIRECTORY_RECORD : FILE_RECORD,
                                   FALSE);
        }
        fill_sector();
        d->size = (cd.sector - d->sector) * SECTOR_SIZE;

        // write directory for joliet
        if (joliet)
        {
            d->joliet_sector = cd.sector;
            write_directory_record(d, DOT_RECORD, TRUE);
            write_directory_record(d == &root ? d : d->parent, DOT_DOT_RECORD, TRUE);
            for (q = d->first_record; q != NULL; q = q->next_in_directory)
            {
                write_directory_record(q,
                                       q->flags & DIRECTORY_FLAG ? SUBDIRECTORY_RECORD : FILE_RECORD,
                                       TRUE);
            }
            fill_sector();
            d->joliet_size = (cd.sector - d->joliet_sector) * SECTOR_SIZE;
            bytes_in_directories += d->joliet_size;
        }

        number_of_directories++;
        bytes_in_directories += d->size;

        // write file data
        for (q = d->first_record; q != NULL; q = q->next_in_directory)
        {
            if ((q->flags & DIRECTORY_FLAG) == 0)
            {
                q->sector = q->joliet_sector = cd.sector;
                size = q->size;
                if (cd.file == NULL)
                {
                    number_of_sectors = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
                    cd.sector += number_of_sectors;
                    number_of_files++;
                    bytes_in_files += size;
                    unused_bytes_at_ends_of_files +=
                    number_of_sectors * SECTOR_SIZE - size;
                }
                else
                {
                    const char *file_source;
                    old_end_source = end_source;
                    if (!q->orig_name)
                    {
                        get_file_specifications(q);
                        *end_source = 0;
                        file_source = source;
                    }
                    else
                    {
                        file_source = q->orig_name;
                    }
                    if (verbosity == VERBOSE)
                        printf("Writing contents of %s\n", file_source);
                    file = fopen(file_source, "rb");
                    if (file == NULL)
                        error_exit("Cannot open %s\n", file_source);
                    if (!write_from_file(file, size))
                    {
                        fclose(file);
                        error_exit("Read error in file %s\n", file_source);
                    }
                    fclose(file);
                    end_source = old_end_source;
                    fill_sector();
                }
            }
        }
    }

    total_sectors = (DWORD)cd.sector;
}

static char HELP[] =
    "\n"
    "CDMAKE CD-ROM Premastering Utility\n"
    "Copyright (C) Philip J. Erdelsky\n"
    "Copyright (C) 2003-2015 ReactOS Team\n"
    "\n\n"
    "CDMAKE [-vN] [-p] [-s N] [-m] [-j] [-pN] [-eN] [-b bootimage]\n"
    "       [-bootdata:N#<defaultBootEntry>#<bootEntry1>#...#<bootEntryN>]\n"
    "       source volume image\n"
    "\n"
    "  -vN           Verbosity level. Valid values for 'N' are:\n"
    "                    0: Quiet mode - display nothing but error messages.\n"
    "                    1: Normal mode (default).\n"
    "                    2: Verbose mode - display file information as files are\n"
    "                       scanned and written. Overrides the -p option.\n"
    "  -p            Show progress while writing.\n"
    "  -s N          Abort operation before beginning write if image will be larger\n"
    "                than N megabytes (i.e. 1024*1024*N bytes).\n"
    "  -m            Accept punctuation marks other than underscores in names and\n"
    "                extensions.\n"
    "  -j            Generate Joliet filename records.\n"
    "\n"
    "  -pN           Boot platform ID in hex format (default: 00 for a BIOS system).\n"
    "  -eN           Boot media emulation. Valid values for 'N' are:\n"
    "                    0 (or nothing): No emulation.\n"
    "                    1: 1.2Mb  diskette.\n"
    "                    2: 1.44Mb diskette.\n"
    "                    3: 2.88Mb diskette.\n"
    "                    4: Hard disk.\n"
    "  -b bootimage  Create a single-boot El-Torito image.\n"
    "  -bootdata:    Create a multi-boot El-Torito image. This option cannot be\n"
    "                combined with the -b option.\n"
    "       Syntax:\n"
    "       -bootdata:N#<defaultBootEntry>#<bootEntry2>#...#<bootEntryN>\n"
    "            'N': number of boot entries following.\n"
    "            defaultBootEntry: The default boot entry, needed in all cases.\n"
    "                Used by BIOSes which do not support additional boot entries.\n"
    "            bootEntryX: Additional boot entries.\n"
    "            - Do not use spaces.\n"
    "            - Each multi-boot entry must be delimited with a hash symbol (#).\n"
    "            - Each option for a boot entry must be delimited with a comma (,).\n"
    "            - Each boot entry must specify the platform ID.\n"
    "\n"
    "  source        Specifications of base directory containing all files to be\n"
    "                written to CD-ROM image.\n"
    "  volume        Volume label.\n"
    "  image         Image file or device.\n";

/*-----------------------------------------------------------------------------
Program execution starts here.
-----------------------------------------------------------------------------*/

char* strtok_s(char *str, const char *delim, char **ctx)
{
    if (delim == NULL || ctx == NULL || (str == NULL && *ctx == NULL))
    {
        return NULL;
    }

    if (!str)
        str = *ctx;

    while (*str && strchr(delim, *str))
        str++;
    if (!*str)
        return NULL;

    *ctx = str + 1;
    while (**ctx && !strchr(delim, **ctx))
        (*ctx)++;
    if (**ctx)
        *(*ctx)++ = '\0';

    return str;
}

int main(int argc, char **argv)
{
    time_t timestamp = time(NULL);
    int i;
    char *t;

    if (argc < 2)
    {
        puts(HELP);
        return 1;
    }

    // Initialize root directory

    cd.buffer = malloc(BUFFER_SIZE);
    if (cd.buffer == NULL)
        error_exit("Insufficient memory");

    memset(&root, 0, sizeof(root));
    root.level = 1;
    root.flags = DIRECTORY_FLAG;
    convert_date_and_time(&root.date_and_time, &timestamp);

    // Initialize CD-ROM write buffer

    cd.file = NULL;
    cd.filespecs[0] = 0;

    // Initialize parameters

    verbosity = NORMAL;
    show_progress = FALSE;
    size_limit = 0;
    accept_punctuation_marks = FALSE;
    source[0] = 0;
    volume_label[0] = 0;

    // Initialize boot information
    eltorito   = FALSE;
    multi_boot = FALSE;
    boot_validation_header.header_id   = 1;  // Validation header ID
    boot_validation_header.platform_id = 0;  // x86/64 BIOS system
    default_boot_entry.boot_id       = 0x88; // Bootable entry
    default_boot_entry.boot_emu_type = 0;    // No emulation
    default_boot_entry.load_segment  = 0;    // 0 --> use default 0x07C0
    default_boot_entry.sector_count  = 0;
    default_boot_entry.load_rba      = 0;
    default_boot_entry.bootimage[0]  = '\0';
    boot_header_list = NULL;

    // Scan command line arguments

    for (i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "-v", 2) == 0)
        {
            t = argv[i] + 2;
            if (*t == 0) // Normal verbosity level.
                verbosity = NORMAL;
            else // Verbosity level in decimal
                verbosity = strtoul(t, NULL, 10);

            // Check for validity
            if (verbosity > VERBOSE)
                verbosity = NORMAL;

            // Disable by default, unless we are in normal verbosity level.
            // If progress is still wanted, use '-p'.
            if (verbosity == QUIET || verbosity == VERBOSE)
                show_progress = FALSE;
        }
        else if (strcmp(argv[i], "-p") == 0)
            show_progress = TRUE;
        else if (strncmp(argv[i], "-s", 2) == 0)
        {
            t = argv[i] + 2;
            if (*t == 0)
            {
                if (++i < argc)
                    t = argv[i];
                else
                    error_exit("Missing size limit parameter");
            }
            // size_limit = strtoul(t, NULL, 10);
            while (isdigit(*t))
                size_limit = size_limit * 10 + *t++ - '0';
            if (size_limit < 1 || size_limit > 800)
                error_exit("Invalid size limit");
            size_limit <<= 9;  // convert megabyte to sector count
        }
        else if (strcmp(argv[i], "-m") == 0)
            accept_punctuation_marks = TRUE;
        else if (strcmp(argv[i], "-j") == 0)
            joliet = TRUE;
        else if (strncmp(argv[i], "-e", 2) == 0)
        {
            // Check whether the multi-boot option '-bootdata:' was already set.
            // If so, print an error and bail out.
            if (eltorito && multi_boot)
                error_exit("Single-boot and multi-boot entries cannot be combined");

            eltorito   = TRUE;
            multi_boot = FALSE;

            t = argv[i] + 2;
            if (*t == 0) // No emulation
                default_boot_entry.boot_emu_type = 0;
            else // ID in decimal
                default_boot_entry.boot_emu_type = (BYTE)strtoul(t, NULL, 10);
        }
        else if (strncmp(argv[i], "-p", 2) == 0)
        {
            // Check whether the multi-boot option '-bootdata:' was already set.
            // If so, print an error and bail out.
            if (eltorito && multi_boot)
                error_exit("Single-boot and multi-boot entries cannot be combined");

            eltorito   = TRUE;
            multi_boot = FALSE;

            // Platform ID in hexadecimal
            boot_validation_header.platform_id = (BYTE)strtoul(argv[i] + 2, NULL, 16);
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            // Check whether the multi-boot option '-bootdata:' was already set.
            // If so, print an error and bail out.
            if (eltorito && multi_boot)
                error_exit("Single-boot and multi-boot entries cannot be combined");

            eltorito   = TRUE;
            multi_boot = FALSE;

            strncpy(default_boot_entry.bootimage, argv[++i], sizeof(default_boot_entry.bootimage));
            default_boot_entry.bootimage[sizeof(default_boot_entry.bootimage)-1] = '\0';
        }
        else if (strncmp(argv[i], "-bootdata:", sizeof("-bootdata:") - 1) == 0)
        {
            char *bootdata, *entry_ctx, *option_ctx;
            DWORD num_boot_entries = 0;

            BOOL default_entry = TRUE; // Start by setting the default boot entry
            PBOOT_HEADER boot_header = NULL; // Current boot header
            PBOOT_ENTRY  boot_entry  = NULL; // The last boot entry in the current boot header
            BYTE platform_id, old_platform_id = 0;
            BYTE boot_emu_type;
            WORD load_segment;
            char bootimage[512];

            // Check whether the single-boot option '-b' was already set.
            // If so, print an error and bail out.
            if (eltorito && !multi_boot)
                error_exit("Single-boot and multi-boot entries cannot be combined");

            t = argv[i] + (sizeof("-bootdata:") - 1);
            bootdata = strdup(t);
            if (bootdata == NULL)
                error_exit("Insufficient memory");

            eltorito   = TRUE;
            multi_boot = TRUE;

            // FIXME: Paths with '#' or ',' or ' ' inside are not yet supported!!

            // Start parsing...
            t = strtok_s(bootdata, "#", &entry_ctx);
            if (t == NULL)
            {
                free(bootdata);
                error_exit("Malformed bootdata command");
            }

            num_boot_entries = strtoul(t, NULL, 10);

            while (num_boot_entries--)
            {
                // Reset to default values
                platform_id   = 0;  // x86/64 BIOS system
                boot_emu_type = 0;  // No emulation
                load_segment  = 0;  // 0 --> use default 0x07C0
                bootimage[0]  = '\0';

                t = strtok_s(NULL, "#", &entry_ctx);
                if (t == NULL)
                {
                    free(bootdata);
                    error_exit("Malformed bootdata command");
                }

                t = strtok_s(t, ",", &option_ctx);
                while (t != NULL)
                {
                    switch (*t++)
                    {
                        case 'b': // Boot sector file
                        {
                            char *q;

                            // Searches for any of the valid separators:
                            // '#' starts a new boot entry;
                            // ',' starts a new boot option;
                            // ' ' finishes the bootdata command.
                            q = strpbrk(t, "#, ");
                            if (!q) q = t + strlen(t);
                            strncpy(bootimage, t, q - t + 1);
                            break;
                        }

                        case 'p': // Platform ID
                        {
                            // Platform ID in hexadecimal
                            platform_id = (BYTE)strtoul(t, NULL, 16);
                            break;
                        }

                        case 'e': // No floppy-disk emulation
                        {
                            if (*t == 0) // No emulation
                                boot_emu_type = 0;
                            else // ID in decimal
                                boot_emu_type = (BYTE)strtoul(t, NULL, 10);

                            break;
                        }

                        case 't': // Loading segment
                        {
                            if (*t == 0) // Not specified --> use default 0x07C0
                                load_segment = 0;
                            else // Segment in hexadecimal
                                load_segment = (BYTE)strtoul(t, NULL, 16);

                            break;
                        }

                        default:
                            free(bootdata);
                            error_exit("Malformed bootdata command");
                    }

                    t = strtok_s(NULL, ",", &option_ctx);
                }

                // Create a new entry and possibly a boot header
                if (default_entry)
                {
                    // Initialize the default boot entry and header

                    boot_validation_header.header_id   = 1;  // Validation header ID
                    boot_validation_header.platform_id = platform_id;
                    default_boot_entry.boot_id       = 0x88; // Bootable entry
                    default_boot_entry.boot_emu_type = boot_emu_type;
                    default_boot_entry.load_segment  = load_segment;

                    strncpy(default_boot_entry.bootimage, bootimage, sizeof(default_boot_entry.bootimage));
                    default_boot_entry.bootimage[sizeof(default_boot_entry.bootimage)-1] = '\0';

                    // Default entry is now initialized.
                    default_entry = FALSE;
                }
                else
                {
                    // Initialize a new boot entry
                    PBOOT_ENTRY old_boot_entry = boot_entry;

                    boot_entry = calloc(1, sizeof(*boot_entry));
                    if (boot_entry == NULL)
                        error_exit("Insufficient memory");
                    // boot_entry->next_entry = NULL;

                    boot_entry->boot_id       = 0x88;   // Bootable entry
                    boot_entry->boot_emu_type = boot_emu_type;
                    boot_entry->load_segment  = load_segment;

                    strncpy(boot_entry->bootimage, bootimage, sizeof(boot_entry->bootimage));
                    boot_entry->bootimage[sizeof(boot_entry->bootimage)-1] = '\0';

                    // Create a new boot header if we don't have one yet
                    if (boot_header == NULL)
                    {
                        boot_header = calloc(1, sizeof(*boot_header));
                        if (boot_header == NULL)
                            error_exit("Insufficient memory");

                        boot_header->header_id   = 0x91; // So far this is the last boot header
                        boot_header->platform_id = platform_id;
                        // boot_header->next_header = NULL;
                        // boot_header->num_entries = 0;
                        // boot_header->entry_list  = NULL;

                        old_boot_entry  = NULL;
                        old_platform_id = platform_id;

                        boot_header_list = boot_header;
                    }
                    else
                    {
                        // Create a new boot header if we change the platform ID
                        if (old_platform_id != platform_id)
                        {
                            PBOOT_HEADER prev_boot_header = boot_header;

                            boot_header = calloc(1, sizeof(*boot_header));
                            if (boot_header == NULL)
                                error_exit("Insufficient memory");

                            boot_header->header_id   = 0x91; // So far this is the last boot header
                            boot_header->platform_id = platform_id;
                            // boot_header->next_header = NULL;
                            // boot_header->num_entries = 0;
                            // boot_header->entry_list  = NULL;

                            old_boot_entry  = NULL;
                            old_platform_id = platform_id;

                            // Link into the header list
                            prev_boot_header->header_id = 0x90; // The previous boot header was not the last one
                            prev_boot_header->next_header = boot_header;
                        }
                    }

                    // Add the entry into the header
                    ++boot_header->num_entries;
                    if (old_boot_entry == NULL)
                        boot_header->entry_list = boot_entry;
                    else
                        old_boot_entry->next_entry = boot_entry;
                }
            }

            free(bootdata);
        }
        else if (i + 2 < argc)
        {
            strcpy(source, argv[i++]);
            strncpy(volume_label, argv[i++], sizeof(volume_label) - 1);
            strcpy(cd.filespecs, argv[i]);
        }
        else
            error_exit("Missing command line argument");
    }

    if (source[0] == 0)
        error_exit("Missing source directory");
    if (volume_label[0] == 0)
        error_exit("Missing volume label");
    if (cd.filespecs[0] == 0)
        error_exit("Missing image file specifications");

    if (source[0] != '@')
    {
        /* set source[] and end_source to source directory,
         * with a terminating directory separator */
        end_source = source + strlen(source);
        if (end_source[-1] == ':')
            *end_source++ = '.';
        if (end_source[-1] != DIR_SEPARATOR_CHAR)
            *end_source++ = DIR_SEPARATOR_CHAR;

        /* scan all files and create directory structure in memory */
        make_directory_records(&root);
    }
    else
    {
        char *trimmedline, *targetname, *srcname, *eq, *normdir;
        char lineread[1024];

        FILE *f = fopen(source+1, "r");
        if (!f)
        {
            error_exit("Cannot open cd description %s\n", source+1);
        }
        while (fgets(lineread, sizeof(lineread), f))
        {
            /* We treat these characters as line endings */
            trimmedline = strtok(lineread, "\t\r\n;");
            eq = strchr(trimmedline, '=');
            if (!eq)
            {
                /* Treat this as a directory name */
                targetname = trimmedline;
                normdir = strdup(targetname);
                normalize_dirname(normdir);
                dir_hash_create_dir(&specified_files, targetname, normdir);
                free(normdir);
            }
            else
            {
                targetname = strtok(lineread, "=");
                srcname = strtok(NULL, "");

#if _WIN32
                if (_access(srcname, R_OK) == 0)
                    dir_hash_add_file(&specified_files, srcname, targetname);
                else
                    error_exit("Cannot access file '%s' (target %s)\n", srcname, targetname);
#else
                if (access(srcname, R_OK) == 0)
                    dir_hash_add_file(&specified_files, srcname, targetname);
                else
                    error_exit("Cannot access file '%s' (target %s)\n", srcname, targetname);
#endif
            }
        }
        fclose(f);

        /* scan all files and create directory structure in memory */
        scan_specified_files(&root, &specified_files.root);
    }

    /* sort path table entries */
    root.next_in_path_table = sort_linked_list(root.next_in_path_table,
                                               1,
                                               compare_path_table_order);

    // initialize CD-ROM write buffer

    cd.file = NULL;
    cd.sector = 0;
    cd.offset = 0;
    cd.count  = 0;

    // make non-writing pass over directory structure to obtain the proper
    // sector numbers and offsets and to determine the size of the image

    number_of_files = bytes_in_files = number_of_directories =
    bytes_in_directories = unused_bytes_at_ends_of_files = 0;
    pass();

    if (verbosity >= NORMAL)
    {
        printf("%s bytes ", edit_with_commas(bytes_in_files, TRUE));
        printf("in %s files\n", edit_with_commas(number_of_files, FALSE));
        printf("%s unused bytes at ends of files\n",
            edit_with_commas(unused_bytes_at_ends_of_files, TRUE));
        printf("%s bytes ", edit_with_commas(bytes_in_directories, TRUE));
        printf("in %s directories\n",
            edit_with_commas(number_of_directories, FALSE));
        printf("%s other bytes\n", edit_with_commas(root.sector * SECTOR_SIZE, TRUE));
        puts("-------------");
        printf("%s total bytes\n",
            edit_with_commas(total_sectors * SECTOR_SIZE, TRUE));
        puts("=============");
    }

    if (size_limit != 0 && total_sectors > size_limit)
        error_exit("Size limit exceeded");

    // re-initialize CD-ROM write buffer

    cd.file = fopen(cd.filespecs, "w+b");
    if (cd.file == NULL)
        error_exit("Cannot open image file %s", cd.filespecs);
    cd.sector = 0;
    cd.offset = 0;
    cd.count  = 0;


    // make writing pass over directory structure

    pass();

    if (cd.count > 0)
        flush_buffer();
    if (show_progress)
        printf("\r             \n");
    if (fclose(cd.file) != 0)
    {
        cd.file = NULL;
        error_exit("File write error in image file %s", cd.filespecs);
    }

    if (verbosity >= NORMAL)
        puts("CD-ROM image made successfully");

    dir_hash_destroy(&specified_files);
    release_memory();
    return 0;
}

/* EOF */
