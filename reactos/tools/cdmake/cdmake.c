/* $Id: cdmake.c,v 1.2 2003/04/11 21:02:21 phreak Exp $ */
/* CD-ROM Maker
   by Philip J. Erdelsky
   pje@acm.org
   http://www.alumni.caltech.edu/~pje/

  ElTorito-Support
  by Eric Kohl
  ekohl@rz-online.de
  */

/* According to his website, this file was released into the public domain by Phillip J. Erdelsky */

#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

const BOOL TRUE  = 1;
const BOOL FALSE = 0;

// file system parameters

#define MAX_LEVEL		8
#define MAX_NAME_LENGTH		8
#define MAX_EXTENSION_LENGTH	3
#define SECTOR_SIZE		2048
#define BUFFER_SIZE		(8 * SECTOR_SIZE)

const BYTE HIDDEN_FLAG    = 1;
const BYTE DIRECTORY_FLAG = 2;


struct cd_image
{
  FILE *file;
  DWORD sector;         // sector to receive next byte
  int offset;           // offset of next byte in sector
  int count;            // number of bytes in buffer
  char filespecs[128];
  BYTE *buffer;
};

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
  char extension[MAX_EXTENSION_LENGTH+1];
  DATE_AND_TIME date_and_time;
  DWORD sector;
  DWORD size;
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

static jmp_buf error;
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

char bootimage[512];
BOOL eltorito;
DWORD boot_catalog_sector;
DWORD boot_image_sector;

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
    s[--i] = x % 10 + '0';
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
  while (root.next_in_memory != NULL)
  {
    struct directory_record *next =
      root.next_in_memory->next_in_memory;
    free (root.next_in_memory);
    root.next_in_memory = next;
  }
  if (cd.buffer != NULL)
  {
    free (cd.buffer);
    cd.buffer = NULL;
  }
}

/*-----------------------------------------------------------------------------
This function edits and displays an error message and then jumps back to the
error exit point in main().
-----------------------------------------------------------------------------*/

static void error_exit(const char *format, ...)
{
  vfprintf(stderr, format, (char*)&format + 1);
  fprintf(stderr, "\n");
  if (cd.file != NULL)
    fclose(cd.file);
  release_memory();
  longjmp(error, 1);
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
  write_byte(x);
  write_byte(x >> 8);
}

static void write_big_endian_word(WORD x)
{
  write_byte(x >> 8);
  write_byte(x);
}

static void write_both_endian_word(WORD x)
{
  write_little_endian_word(x);
  write_big_endian_word(x);
}

static void write_little_endian_dword(DWORD x)
{
  write_byte(x);
  write_byte(x >> 8);
  write_byte(x >> 16);
  write_byte(x >> 24);
}

static void write_big_endian_dword(DWORD x)
{
  write_byte(x >> 24);
  write_byte(x >> 16);
  write_byte(x >> 8);
  write_byte(x);
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
This function writes a directory record to the CD_ROM image.
-----------------------------------------------------------------------------*/

static void
write_directory_record(PDIR_RECORD d,
		       DIR_RECORD_TYPE  DirType)
{
  unsigned identifier_size;
  unsigned record_size;

  switch (DirType)
  {
    case DOT_RECORD:
    case DOT_DOT_RECORD:
      identifier_size = 1;
      break;
    case SUBDIRECTORY_RECORD:
      identifier_size = strlen(d->name);
      break;
    case FILE_RECORD:
      identifier_size = strlen(d->name) + 2;
      if (d->extension[0] != 0)
        identifier_size += 1 + strlen(d->extension);
      break;
  }
  record_size = 33 + identifier_size;
  if ((identifier_size & 1) == 0)
    record_size++;
  if (cd.offset + record_size > SECTOR_SIZE)
    fill_sector();
  write_byte(record_size);
  write_byte(0); // number of sectors in extended attribute record
  write_both_endian_dword(d->sector);
  write_both_endian_dword(d->size);
  write_byte(d->date_and_time.year - 1900);
  write_byte(d->date_and_time.month);
  write_byte(d->date_and_time.day);
  write_byte(d->date_and_time.hour);
  write_byte(d->date_and_time.minute);
  write_byte(d->date_and_time.second);
  write_byte(0);  // GMT offset
  write_byte(d->flags);
  write_byte(0);  // file unit size for an interleaved file
  write_byte(0);  // interleave gap size for an interleaved file
  write_both_endian_word((WORD) 1); // volume sequence number
  write_byte(identifier_size);
  switch (DirType)
  {
    case DOT_RECORD:
      write_byte(0);
      break;
    case DOT_DOT_RECORD:
      write_byte(1);
      break;
    case SUBDIRECTORY_RECORD:
      write_string(d->name);
      break;
    case FILE_RECORD:
      write_string(d->name);
      if (d->extension[0] != 0)
      {
        write_byte('.');
        write_string(d->extension);
      }
      write_string(";1");
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

  timedef = localtime(time);

  dt->second = timedef->tm_sec;
  dt->minute = timedef->tm_min;
  dt->hour = timedef->tm_hour;
  dt->day = timedef->tm_mday;
  dt->month = timedef->tm_mon;
  dt->year = timedef->tm_year + 1900;
}

/*-----------------------------------------------------------------------------
This function checks the specified character, if necessary, and
generates an error if it is a punctuation mark other than an underscore.
It also converts small letters to capital letters and returns the
result.
-----------------------------------------------------------------------------*/

static int check_for_punctuation(int c, char *name)
{
  c = toupper(c & 0xFF);
  if (!accept_punctuation_marks && !isalnum(c) && c != '_')
    error_exit("Punctuation mark in %s", name);
  return c;
}

/*-----------------------------------------------------------------------------
This function creates a new directory record with the information from the
specified ffblk. It links it into the beginning of the directory list
for the specified parent and returns a pointer to the new record.
-----------------------------------------------------------------------------*/

PDIR_RECORD
new_directory_record (struct _finddata_t *f,
		      PDIR_RECORD parent)
{
  PDIR_RECORD d;
  char *s;
  char *t;

  d = malloc(sizeof(DIR_RECORD));
  if (d == NULL)
    error_exit("Insufficient memory");
  d->next_in_memory = root.next_in_memory;
  root.next_in_memory = d;
  {
    s = f->name;
    t = d->name;
    while (*s != 0)
    {
      if (*s == '.')
      {
        s++;
        break;
      }
      *t++ = check_for_punctuation(*s++, f->name);
    }
    *t = 0;
    t = d->extension;
    while (*s != 0)
      *t++ = check_for_punctuation(*s++, f->name);
    *t = 0;
  }
  convert_date_and_time(&d->date_and_time, &f->time_create);
  if (f->attrib & _A_SUBDIR)
  {
    if (d->extension[0] != 0)
      error_exit("Directory with extension %s", f->name);
    d->flags = DIRECTORY_FLAG;
  }
  else
    d->flags = f->attrib & _A_HIDDEN ? HIDDEN_FLAG : 0;
  d->size = f->size;
  d->next_in_directory = parent->first_record;
  parent->first_record = d;
  d->parent = parent;
  return d;
}

/*-----------------------------------------------------------------------------
This function compares two directory records according to the ISO9660 rules
for directory sorting and returns a negative value if p is before q, or a
positive value if p is after q.
-----------------------------------------------------------------------------*/

static int compare_directory_order(PDIR_RECORD p, PDIR_RECORD q)
{
  int n = strcmp(p->name, q->name);
  if (n == 0)
    n = strcmp(p->extension, q->extension);
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

static void
make_directory_records (PDIR_RECORD d)
{
  PDIR_RECORD new_d;
  struct _finddata_t f;
  char *old_end_source;
  int findhandle;

  d->first_record = NULL;
  strcpy(end_source, "*.*");

  findhandle =_findfirst(source, &f);
  if (findhandle != 0)
    {
      do
	{
	  if ((f.attrib & (_A_HIDDEN | _A_SUBDIR)) == 0 && f.name[0] != '.')
	    {
	      if (strcmp(f.name, DIRECTORY_TIMESTAMP) == 0)
		{
		  convert_date_and_time(&d->date_and_time, &f.time_create);
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
  findhandle= _findfirst(source, &f);
  if (findhandle)
    {
      do
	{
	  if (f.attrib & _A_SUBDIR && f.name[0] != '.')
	    {
	      old_end_source = end_source;
	      append_string_to_source(f.name);
	      *end_source++ = '\\';
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
    append_string_to_source(d->name);
    if ((d->flags & DIRECTORY_FLAG) == 0 && d->extension[0] != 0)
    {
      *end_source++ = '.';
      append_string_to_source(d->extension);
    }
    if (d->flags & DIRECTORY_FLAG)
      *end_source++ = '\\';
  }
}

static void pass(void)
{
  PDIR_RECORD d;
  PDIR_RECORD q;
  unsigned int i;
  char *t;
  unsigned int index;
  unsigned int name_length;
  DWORD size;
  DWORD number_of_sectors;
  char *old_end_source;
  int n;
  FILE *file;

  // first 16 sectors are zeros

  write_block(16 * SECTOR_SIZE, 0);

  // Primary Volume Descriptor

  write_string("\1CD001\1");
  write_byte(0);
  write_block(32, ' ');  // system identifier

  t = volume_label;
  for (i = 0; i < 32; i++)
    write_byte(*t != 0 ? toupper(*t++) : ' ');

  write_block(8, 0);
  write_both_endian_dword(total_sectors);
  write_block(32, 0);
  write_both_endian_word((WORD) 1); // volume set size
  write_both_endian_word((WORD) 1); // volume sequence number
  write_both_endian_word((WORD) 2048); // sector size
  write_both_endian_dword(path_table_size);
  write_little_endian_dword(little_endian_path_table_sector);
  write_little_endian_dword((DWORD) 0);  // second little endian path table
  write_big_endian_dword(big_endian_path_table_sector);
  write_big_endian_dword((DWORD) 0);  // second big endian path table
  write_directory_record(&root, DOT_RECORD);
  write_block(128, ' ');      // volume set identifier
  write_block(128, ' ');      // publisher identifier
  write_block(128, ' ');      // data preparer identifier
  write_block(128, ' ');      // application identifier
  write_block(37, ' ');       // copyright file identifier
  write_block(37, ' ');       // abstract file identifier
  write_block(37, ' ');       // bibliographic file identifier
  write_string("0000000000000000");  // volume creation
  write_byte(0);
  write_string("0000000000000000");  // most recent modification
  write_byte(0);
  write_string("0000000000000000");  // volume expires
  write_byte(0);
  write_string("0000000000000000");  // volume is effective
  write_byte(0);
  write_byte(1);
  write_byte(0);
  fill_sector();


  // Boot Volume Descriptor

  if (eltorito == TRUE)
    {
      write_byte(0);
      write_string("CD001\1");
      write_string("EL TORITO SPECIFICATION");  // identifier
      write_block(9, 0);  // padding
      write_block(32, 0);  // unused
      write_little_endian_dword(boot_catalog_sector);  // pointer to boot catalog
      fill_sector();
    }


  // Volume Descriptor Set Terminator

  write_string("\377CD001\1");
  fill_sector();


  // Boot Catalog

  if (eltorito == TRUE)
    {
      boot_catalog_sector = cd.sector;

      // Validation entry
      write_byte(1);
      write_byte(0);  // x86 boot code
      write_little_endian_word(0);  // reserved
      write_string("ReactOS Foundation");
      write_block(6, 0); // padding
      write_little_endian_word(0x62E);  // checksum
      write_little_endian_word(0xAA55);  // signature

      // default entry
      write_byte(0x88);  // bootable
      write_byte(0);  // no emulation
      write_big_endian_word(0);  // load segment = default (0x07c0)
      write_byte(0);  // partition type
      write_byte(0);  // unused
      write_little_endian_word(4);  // sector count
      write_little_endian_dword(boot_image_sector);  // sector count

      fill_sector();
    }


  // Boot Image

  if (eltorito == TRUE)
    {
      boot_image_sector = cd.sector;

      file = fopen(bootimage, "rb");
      if (file == NULL)
        error_exit("Can't open %s\n", bootimage);
      fseek(file, 0, SEEK_END);
      size = ftell(file);
      fseek(file, 0, SEEK_SET);
      while (size > 0)
      {
        n = BUFFER_SIZE - cd.count;
        if ((DWORD) n > size)
          n = size;
        if (fread (cd.buffer + cd.count, n, 1, file) < 1)
        {
          fclose(file);
          error_exit("Read error in file %s\n", bootimage);
        }
        cd.count += n;
        if (cd.count == BUFFER_SIZE)
          flush_buffer();
        cd.sector += n / SECTOR_SIZE;
        cd.offset += n % SECTOR_SIZE;
        size -= n;
      }
      fclose(file);
//      fill_sector();
    }


  // Little Endian Path Table

  little_endian_path_table_sector = cd.sector;
  write_byte(1);
  write_byte(0);  // number of sectors in extended attribute record
  write_little_endian_dword(root.sector);
  write_little_endian_word((WORD) 1);
  write_byte(0);
  write_byte(0);

  index = 1;
  root.path_table_index = 1;
  for (d = root.next_in_path_table; d != NULL; d = d->next_in_path_table)
    {
      name_length = strlen(d->name);
      write_byte(name_length);
      write_byte(0);  // number of sectors in extended attribute record
      write_little_endian_dword(d->sector);
      write_little_endian_word(d->parent->path_table_index);
      write_string(d->name);
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
  write_big_endian_word((WORD) 1);
  write_byte(0);
  write_byte(0);

  for (d = root.next_in_path_table; d != NULL; d = d->next_in_path_table)
    {
      name_length = strlen(d->name);
      write_byte(name_length);
      write_byte(0);  // number of sectors in extended attribute record
      write_big_endian_dword(d->sector);
      write_big_endian_word(d->parent->path_table_index);
      write_string(d->name);
      if (name_length & 1)
        write_byte(0);
    }
  fill_sector();


  // directories and files

  for (d = &root; d != NULL; d = d->next_in_path_table)
    {
      // write directory

      d->sector = cd.sector;
      write_directory_record(d, DOT_RECORD);
      write_directory_record(d == &root ? d : d->parent, DOT_DOT_RECORD);
      for (q = d->first_record; q != NULL; q = q->next_in_directory)
	{
	  write_directory_record(q,
				 q->flags & DIRECTORY_FLAG ? SUBDIRECTORY_RECORD : FILE_RECORD);
	}
      fill_sector();
      d->size = (cd.sector - d->sector) * SECTOR_SIZE;
      number_of_directories++;
      bytes_in_directories += d->size;

      // write file data

      for (q = d->first_record; q != NULL; q = q->next_in_directory)
      {
        if ((q->flags & DIRECTORY_FLAG) == 0)
        {
          q->sector = cd.sector;
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
            old_end_source = end_source;
            get_file_specifications(q);
            *end_source = 0;
            if (verbosity == VERBOSE)
              printf("Writing %s\n", source);
            file = fopen(source, "rb");
            if (file == NULL)
              error_exit("Can't open %s\n", source);
            fseek(file, 0, SEEK_SET);
            while (size > 0)
            {
              n = BUFFER_SIZE - cd.count;
              if ((DWORD) n > size)
                n = size;
              if (fread (cd.buffer + cd.count, n, 1, file) < 1)
              {
                fclose(file);
                error_exit("Read error in file %s\n", source);
              }
              cd.count += n;
              if (cd.count == BUFFER_SIZE)
                flush_buffer();
              cd.sector += n / SECTOR_SIZE;
              cd.offset += n % SECTOR_SIZE;
              size -= n;
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
  "CDMAKE  [-q] [-v] [-p] [-s N] [-m] [-b bootimage]  source  volume  image\n"
  "\n"
  "  source        specifications of base directory containing all files to\n"
  "                be written to CD-ROM image\n"
  "  volume        volume label\n"
  "  image         image file or device\n"
  "  -q            quiet mode - display nothing but error messages\n"
  "  -v            verbose mode - display file information as files are\n"
  "                scanned and written - overrides -p option\n"
  "  -p            show progress while writing\n"
  "  -s N          abort operation before beginning write if image will be\n"
  "                larger than N megabytes (i.e. 1024*1024*N bytes)\n"
  "  -m            accept punctuation marks other than underscores in\n"
  "                names and extensions\n"
  "  -b bootimage  create bootable ElTorito CD-ROM using 'no emulation' mode\n";

/*-----------------------------------------------------------------------------
Program execution starts here.
-----------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  BOOL q_option = FALSE;
  BOOL v_option = FALSE;
  int i;
  char *t;

  if (argc < 2)
  {
    puts(HELP);
    return 1;
  }

  if (setjmp(error))
    return 1;

  // initialize root directory

  cd.buffer = malloc (BUFFER_SIZE);
  if (cd.buffer == NULL)
    error_exit("Insufficient memory");

  memset(&root, 0, sizeof(root));
  root.level = 1;
  root.flags = DIRECTORY_FLAG;

  // initialize CD-ROM write buffer

  cd.file = NULL;
  cd.filespecs[0] = 0;

  // initialize parameters

  verbosity = NORMAL;
  size_limit = 0;
  show_progress = FALSE;
  accept_punctuation_marks = FALSE;
  source[0] = 0;
  volume_label[0] = 0;

  eltorito = FALSE;

  // scan command line arguments

  for (i = 1; i < argc; i++)
    {
      if (memcmp(argv[i], "-s", 2) == 0)
      {
        t = argv[i] + 2;
        if (*t == 0)
        {
          if (++i < argc)
            t = argv[i];
          else
            error_exit("Missing size limit parameter");
        }
        while (isdigit(*t))
          size_limit = size_limit * 10 + *t++ - '0';
        if (size_limit < 1 || size_limit > 800)
          error_exit("Invalid size limit");
        size_limit <<= 9;  // convert megabyte to sector count
      }
      else if (strcmp(argv[i], "-q") == 0)
        q_option = TRUE;
      else if (strcmp(argv[i], "-v") == 0)
        v_option = TRUE;
      else if (strcmp(argv[i], "-p") == 0)
        show_progress = TRUE;
      else if (strcmp(argv[i], "-m") == 0)
        accept_punctuation_marks = TRUE;
      else if (strcmp(argv[i], "-b") == 0)
      {
        strcpy(bootimage, argv[++i]);
        eltorito = TRUE;
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
  if (v_option)
    {
      show_progress = FALSE;
      verbosity = VERBOSE;
    }
  else if (q_option)
    {
      verbosity = QUIET;
      show_progress = FALSE;
    }
  if (source[0] == 0)
    error_exit("Missing source directory");
  if (volume_label[0] == 0)
    error_exit("Missing volume label");
  if (cd.filespecs[0] == 0)
    error_exit("Missing image file specifications");


  // set source[] and end_source to source directory, with a terminating \

  end_source = source + strlen(source);
  if (end_source[-1] == ':')
    *end_source++ = '.';
  if (end_source[-1] != '\\')
    *end_source++ = '\\';

  // scan all files and create directory structure in memory

  make_directory_records(&root);

  // sort path table entries

  root.next_in_path_table = sort_linked_list(root.next_in_path_table, 1,
    compare_path_table_order);

  // initialize CD-ROM write buffer

  cd.file = NULL;
  cd.sector = 0;
  cd.offset = 0;
  cd.count = 0;

  // make non-writing pass over directory structure to obtain the proper
  // sector numbers and offsets and to determine the size of the image

  number_of_files = bytes_in_files = number_of_directories =
    bytes_in_directories = unused_bytes_at_ends_of_files =0;
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
    error_exit("Can't open image file %s", cd.filespecs);
  cd.sector = 0;
  cd.offset = 0;
  cd.count = 0;


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

  release_memory();
  return 0;
}

/* EOF */
