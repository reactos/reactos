/*
 * Convert WINE makefiles to ReactOS makefiles.
 * Casper S. Hornstrup <chorns@users.sourceforge.net>
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <io.h>
#include <dos.h>
#define strcasecmp strcmpi
#else
#include <sys/io.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include <ctype.h>
#ifndef WIN32
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif

typedef struct _KEY_VALUE_INFO
{
  struct _KEY_VALUE_INFO *next;
  char filename[MAX_PATH];
} KEY_VALUE_INFO, *PKEY_VALUE_INFO;

typedef struct _KEY_INFO
{
  struct _KEY_INFO *next;
  char keyname[200];
  PKEY_VALUE_INFO keyvalueinfo;
  PKEY_VALUE_INFO lastvalue;
} KEY_INFO, *PKEY_INFO;

typedef struct _MAKEFILE_INFO
{
  struct _MAKEFILE_INFO *next;
  PKEY_INFO keyinfo;
} MAKEFILE_INFO, *PMAKEFILE_INFO;

static FILE *out;
static FILE *file_handle = NULL;
static char *file_buffer = NULL;
static unsigned int file_size = 0;
static int file_pointer = 0;
static char name[255];
static char value[4096];
static char *template_file;


static char*
convert_path(char* origpath)
{
   char* newpath;
   int i;
   
   newpath = strdup(origpath);
   
   i = 0;
   while (newpath[i] != 0)
     {
#ifndef WIN32
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef WIN32
	if (newpath[i] == '/')
	  {
	     newpath[i] = '\\';
	  }
#endif	
#endif	
	i++;
     }
   return(newpath);
}

static void
write_line(char *line)
{
  int n_out;
  char buf[2000];

  memset(buf, 0, sizeof(buf));
  strcpy(buf, line);
  /* Terminate the line */
  buf[strlen(buf)] = '\r';
  buf[strlen(buf)] = '\n';

  n_out = fwrite(&buf[0], 1, strlen(buf), out);
}


static void
read_file(char *filename)
{
  file_handle = fopen(filename, "rb");
  if (file_handle == NULL)
    {
      printf("Can't open %s\n", filename);
      exit(1);
    }

  // Get the size of the file
  fseek(file_handle, 0, SEEK_END);
  file_size = ftell(file_handle);

  // Load it all into memory
  file_buffer = malloc(file_size);
  if (file_buffer == NULL)
    {
      fclose(file_handle);
      printf("Out of memory\n");
      exit(1);
    }
  fseek(file_handle, 0, SEEK_SET);
  if (file_size > 0)
    {
      if (fread (file_buffer, 1, file_size, file_handle) < 1)
        {
          fclose(file_handle);
          printf("Read error in file %s\n", filename);
          exit(1);
        }
    }

  file_pointer = 0;
}

static void
close_file()
{
  free(file_buffer);
  file_buffer = NULL;
  fclose(file_handle);
  file_handle = NULL;
  file_pointer = 0;
}

static int
is_whitespace(char ch)
{
  if (ch == ' ')
    {
      return 1;
    }
  if (ch == '\t')
    {
      return 1;
    }
  return 0;
}

static int
is_eol_char(char ch)
{
  if (ch == '\r')
    {
      return 1;
    }
  if (ch == '\n')
    {
      return 1;
    }
  return 0;
}

static int
is_end_of_name(char ch)
{
  if ((ch >= 'a') && (ch <= 'z'))
    {
      return 0;
    }
  if ((ch >= 'A') && (ch <= 'Z'))
    {
      return 0;
    }
  if ((ch >= '0') && (ch <= '9'))
    {
      return 0;
    }
  if (ch == '_')
    {
      return 0;
    }
  return 1;
}

static int
is_end_of_list(int line_continuation_character_found)
{
  if (is_eol_char(file_buffer[file_pointer]) && !line_continuation_character_found)
    {
      return 1;
    }

  return 0;
}

static void
skip_line()
{
  while ((file_pointer < file_size) && (!is_eol_char(file_buffer[file_pointer])))
    {
      file_pointer++;
    }
  if ((file_pointer < file_size) && (is_eol_char(file_buffer[file_pointer])))
    {
      file_pointer++;
      if ((file_pointer < file_size) && (file_buffer[file_pointer] == '\n'))
        {
          file_pointer++;
        }
    }
}

static void
skip_whitespace()
{
  while ((file_pointer < file_size) && !is_eol_char(file_buffer[file_pointer])
    && is_whitespace(file_buffer[file_pointer]))
    {
      file_pointer++;
    }
}

static void
skip_comments()
{
  while ((file_pointer < file_size))
    {
      skip_whitespace();
      if (file_buffer[file_pointer] == '#')
        {
          skip_line();
        }
      else
        {
          break;
        }
    }
}

static int
get_identifier()
{
  unsigned int begin_pointer;
  int len;

  name[0] = 0;

  /* Skip any remaining whitespace */
  skip_whitespace();

  begin_pointer = file_pointer;
  while ((file_pointer < file_size))
    {
      if (is_end_of_name(file_buffer[file_pointer]))
        {
          len = file_pointer - begin_pointer;
          strncpy(name, &file_buffer[begin_pointer], len);
          name[len] = 0;
          return 1;
        }
      file_pointer++;
    }

  return 0;
}

static int
get_value()
{
  int line_continuation_character_found;
  unsigned int begin_pointer;
  int len;
  int i;
  int j;

  value[0] = 0;

  /* Skip any remaining whitespace */
  skip_whitespace();

  line_continuation_character_found = (file_buffer[file_pointer] == '\\');
  begin_pointer = file_pointer;
  while ((file_pointer < file_size))
    {
      if (is_end_of_list(line_continuation_character_found))
        {
          len = file_pointer - begin_pointer;
          j = 0;
          for (i = begin_pointer; i < file_pointer; i++)
            {
              if (file_buffer[i] != '\\')
                {
                  if ((!is_eol_char(file_buffer[i])) && (((j == 0) || (!is_whitespace(file_buffer[i])) ||
                    (is_whitespace(file_buffer[i]) && !is_whitespace(value[j-1])))))
                    {
                      if ((j != 0) || (!is_whitespace(file_buffer[i])))
                        {
                          value[j] = file_buffer[i];
                          j++;
                        }
                    }
                }
            }
          value[j] = 0;
          return 1;
        }
      if (!is_eol_char(file_buffer[file_pointer]) && !is_whitespace(file_buffer[file_pointer]))
        {
          line_continuation_character_found = 0;
        }
      if (file_buffer[file_pointer] == '\\')
        {
          line_continuation_character_found = 1;
        }
      file_pointer++;
    }

  return 0;
}

static int
read_next_string(char **entry, char *string)
{
  char *p1 = *entry;
  char *p2;

  if (*p1 == 0)
    {
      return 0;
    }

  p2 = p1;
  while ((*p1 != 0) && (!is_whitespace(*p1)))
    {
      p1++;
    }

  if (((p1 - p2) == 0))
    {
      return 0;
    }

  strncpy(string, p2, p1 - p2);
  string[p1 - p2] = 0;

  while ((*p1 != 0) && (is_whitespace(*p1)))
    {
      p1++;
    }

  *entry = p1;

  return 1;
}

static void
parse_key_value(PKEY_INFO key_info)
{
  char string[MAX_PATH];
  char *entry;
  PKEY_VALUE_INFO keyvalue_info;

  get_value();
  entry = &value[0];
  while (read_next_string(&entry, string))
    {
      keyvalue_info = malloc(sizeof(KEY_VALUE_INFO));
      if (keyvalue_info == NULL)
        {
          printf("Out of memory\n");
          exit(1);
        }
      memset(keyvalue_info, 0, sizeof(KEY_VALUE_INFO));
      strcpy(keyvalue_info->filename, string);
      if (NULL != key_info->lastvalue)
        {
          key_info->lastvalue->next = keyvalue_info;
        }
      else
        {
          key_info->keyvalueinfo = keyvalue_info;
        }
      key_info->lastvalue = keyvalue_info;
    }
  skip_line();
}

static PMAKEFILE_INFO
parse_input_file(char *filename)
{
  PMAKEFILE_INFO makefile_info;
  PKEY_INFO key_info;

  read_file(filename);

  makefile_info = malloc(sizeof(MAKEFILE_INFO));
  if (makefile_info == NULL)
    {
      printf("Out of memory\n");
      exit(1);
    }
  memset(makefile_info, 0, sizeof(MAKEFILE_INFO));

  do
    {
      if (file_pointer >= file_size)
        {
          break;
        }

      if (is_eol_char(file_buffer[file_pointer]))
        {
          skip_line();
        }

      /* Skip any comments that might be */
      skip_comments();

      if (is_eol_char(file_buffer[file_pointer]))
        {
          skip_line();
        }

      if (get_identifier())
        {
          /* Skip any whitespace */
          skip_whitespace();
          if (file_buffer[file_pointer] == '=')
            {
              file_pointer++;
              /* Skip any whitespace */
              skip_whitespace();

              key_info = malloc(sizeof(KEY_INFO));
              if (key_info == NULL)
                {
                  printf("Out of memory\n");
                  exit(1);
                }
              memset(key_info, 0, sizeof(KEY_INFO));
              strcpy(key_info->keyname, name);
              key_info->next = makefile_info->keyinfo;
              makefile_info->keyinfo = key_info;
              parse_key_value(key_info);
            }
          else
            {
              skip_line();
            }
        }
    } while (1);

  close_file();

  return makefile_info;
}

static PKEY_INFO
find_key(PMAKEFILE_INFO makefile_info, char *keyname)
{
  PKEY_INFO key_info;

  for (key_info = makefile_info->keyinfo; key_info != NULL; key_info = key_info->next)
    {
      if (strcasecmp(key_info->keyname, keyname) == 0)
        {
          return key_info;
        }
    }
  return NULL;
}

static void
change_extension(char *filenamebuffer, char *filename, char *newextension)
{
  char *ptr;

  if (newextension == NULL)
    {
      strcpy(filenamebuffer, filename);
      return;
    }

  ptr = strrchr(filename, '.');
  if (ptr != NULL)
    {
      strncpy(filenamebuffer, filename, ptr - filename);
      filenamebuffer[ptr - filename] = 0;
      strcat(filenamebuffer, newextension);
    }
  else
    {
      strcpy(filenamebuffer, filename);
      strcat(filenamebuffer, newextension);
    }
}

static void
replace_identifier(PMAKEFILE_INFO makefile_info, char *identifier, char *buffer,
  int *buffer_pointer, char *newextension)
{
  PKEY_INFO key_info;
  PKEY_VALUE_INFO keyvalue_info;
  int count;
  int i;
  char filenamebuffer[MAX_PATH];

  key_info = find_key(makefile_info, identifier);
  if (key_info == NULL)
    {
      return;
    }

  count = 0;
  for (keyvalue_info = key_info->keyvalueinfo; keyvalue_info != NULL; keyvalue_info = keyvalue_info->next)
    {
      if (count > 0)
        {
          buffer[*buffer_pointer] = ' ';
          *buffer_pointer = *buffer_pointer + 1;
        }

      change_extension(filenamebuffer, keyvalue_info->filename, newextension);
      for (i = 0; i < strlen(filenamebuffer); i++)
        {
          buffer[*buffer_pointer] = filenamebuffer[i];
          *buffer_pointer = *buffer_pointer + 1;
        }
      count++;
    }
}

static char *get_extension(char *keyname)
{
  if (strcasecmp(keyname, "C_SRCS") == 0)
    {
      return ".o";
    }
  else if (strcasecmp(keyname, "IMPORTS") == 0)
    {
      return ".a";
    }
  else
    {
      return NULL;
    }
}

static void
replace_identifiers(PMAKEFILE_INFO makefile_info)
{
  char buffer[4096];
  int buffer_pointer;

  buffer[0] = 0;
  buffer_pointer = 0;
  while ((file_pointer < file_size) && (!is_eol_char(file_buffer[file_pointer])))
    {
      if (file_buffer[file_pointer] == '@')
        {
           file_pointer++;
           if (get_identifier() && file_buffer[file_pointer] == '@')
             {
               file_pointer++;
               replace_identifier(makefile_info, name, buffer, &buffer_pointer, get_extension(name));
             }
           else if (file_buffer[file_pointer] != '@')
             {
               if (buffer_pointer < sizeof(buffer))
                 {
                   buffer[buffer_pointer] = file_buffer[file_pointer];
                   file_pointer++;
                   buffer_pointer++;
                 }
             }
           else if (buffer_pointer < sizeof(buffer))
             {
               buffer[buffer_pointer] = file_buffer[file_pointer];
               file_pointer++;
               buffer_pointer++;
             }
           else
             {
               break;
             }
        }
      else if (buffer_pointer < sizeof(buffer))
        {
          buffer[buffer_pointer] = file_buffer[file_pointer];
          file_pointer++;
          buffer_pointer++;
        }
      else
        {
          break;
        }
    }

  if (is_eol_char(file_buffer[file_pointer]))
    {
      skip_line();
    }
  buffer[buffer_pointer] = 0;

  write_line(buffer);
}

static void
expand_line(PMAKEFILE_INFO makefile_info)
{
  replace_identifiers(makefile_info);
}

static void
write_makefile(PMAKEFILE_INFO makefile_info)
{
  read_file(template_file);

  do
    {
      if (file_pointer >= file_size)
        {
          break;
        }

      expand_line(makefile_info);

    } while (1);

  close_file();
}

static char HELP[] =
  "WINE2ROS wine-makefile template-makefile reactos-makefile\n"
  "\n"
  "  wine-makefile     WINE makefile to convert\n"
  "  template-makefile ReactOS template makefile to use\n"
  "  reactos-makefile  ReactOS makefile to be generated\n";

int main(int argc, char **argv)
{
  char *input_file;
  char *output_file;
  PMAKEFILE_INFO makefile_info;

  if (argc < 4)
    {
      puts(HELP);
      return 1;
    }

  input_file = convert_path(argv[1]);
  if (input_file[0] == 0)
    {
      printf("Missing input-filename\n");
      return 1;
    }

  template_file = convert_path(argv[2]);

  output_file = convert_path(argv[3]);
  if (output_file[0] == 0)
    {
      printf("Missing output-filename\n");
      return 1;
    }

  out = fopen(output_file, "wb");
  if (out == NULL)
    {
    	printf("Cannot open output file");
    	return 1;
     }

  makefile_info = parse_input_file(input_file);
  if (makefile_info != NULL)
    {
      write_makefile(makefile_info);
    }

  fclose(out);

  return 0;
}
