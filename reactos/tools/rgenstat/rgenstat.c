/*
 * Generate a file with API status information from a list
 * of files in a directory.
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
#else
#include <sys/io.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
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

#define TAG_UNKNOWN -1
#define TAG_IMPLEMENTED 0
#define TAG_UNIMPLEMENTED 1

typedef struct _API_INFO
{
  struct _API_INFO *next;
  int tag_id;
  char name[100];
  char filename[MAX_PATH];
} API_INFO, *PAPI_INFO;


PAPI_INFO sort_linked_list(PAPI_INFO,
    unsigned, int (*)(PAPI_INFO, PAPI_INFO));


static FILE *in;
static FILE *out;
static char *file;
static FILE *file_handle = NULL;
static char *file_buffer = NULL;
static unsigned int file_size = 0;
static int file_pointer = 0;
static char tagname[200];
static PAPI_INFO api_info_list = NULL;


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

static char*
path_to_url(char* path)
{
   int i;
      
   i = 0;
   while (path[i] != 0)
     {
	if (path[i] == '\\')
	  {
	     path[i] = '/';
	  }
	i++;
     }
   return(path);
}

static void
write_line(char *line)
{
  int n_out;
  char buf[200];

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
is_end_of_tag(char ch)
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
is_end_of_name(char ch)
{
  /* Currently the same as is_end_of_tag() */
  return is_end_of_tag(ch);
}

static int
is_valid_file(char *filename)
{
  char ext[MAX_PATH];
  int i;

  i = strlen(filename);
  while (i > 0 && filename[i] != '.')
    {
      i--;
    }
  if (i > 0)
    {
      memset(ext, 0, sizeof(ext));
      strncpy(&ext[0], &filename[i], strlen(&filename[i]));

      if ((strncmp(ext, ".c", 2) == 0) || (strncmp(ext, ".C", 2) == 0))
        {
          return 1;
        }
    }
  return 0;
}

static int
get_tag_id(char *tag)
{
  if (strcasecmp(tag, "implemented") == 0)
    {
      return TAG_IMPLEMENTED;
    }
  if (strcasecmp(tag, "unimplemented") == 0)
    {
      return TAG_UNIMPLEMENTED;
    }
  return TAG_UNKNOWN;
}

static int
skip_to_next_tag()
{
  unsigned int start;
  int end_of_tag;
  int found_tag = 0;
  int tag_id;
  int len;

  tagname[0] = 0;
  while ((file_pointer < file_size) && (!found_tag))
    {
      if (file_buffer[file_pointer] == '@')
        {
          file_pointer++;
          start = file_pointer;
          end_of_tag = 0;
          while ((file_pointer < file_size) && (!end_of_tag))
            {
              end_of_tag = is_end_of_tag(file_buffer[file_pointer]);
              file_pointer++;
            }
          len = file_pointer > start ? file_pointer - start - 1 : 0;
          strncpy(tagname, &file_buffer[start], len);
          tagname[len] = 0;

          tag_id = get_tag_id(tagname);
          if (tag_id != TAG_UNKNOWN)
            {
              return tag_id;
            }
        }
      file_pointer++;
    }

  return TAG_UNKNOWN;
}

static void
skip_line()
{
  while ((file_pointer < file_size) && (!is_eol_char(file_buffer[file_pointer])))
    {
      file_pointer++;
    }
  if ((file_pointer < file_size) && (file_buffer[file_pointer] == '\n'))
    {
      file_pointer++;
    }
}

static void
skip_comments()
{
  while ((file_pointer < file_size))
    {
      if (file_buffer[file_pointer] == '*')
        {
          if ((file_pointer + 1 < file_size))
            {
              if (file_buffer[file_pointer + 1] == '/')
                {
                  skip_line();
                  return;
                }
            }
        }
      file_pointer++;
    }
}

static int
get_previous_identifier(unsigned int end, char *name)
{
  unsigned int my_file_pointer = end;
  int len;

  name[0] = 0;

  while ((my_file_pointer > 0) && (is_whitespace(file_buffer[my_file_pointer])
    || is_eol_char(file_buffer[my_file_pointer])))
    {
      my_file_pointer--;
    }

  /* Skip any comments between function name and it's parameters */
  if ((my_file_pointer > 0) && (file_buffer[my_file_pointer] == '/'))
    {
      if ((my_file_pointer > 0) && (file_buffer[my_file_pointer - 1] == '*'))
        {
          my_file_pointer--;
          while ((my_file_pointer > 0) && !((file_buffer[my_file_pointer] == '*')
            && (file_buffer[my_file_pointer - 1] == '/')))
            {
              my_file_pointer--;
            }
          my_file_pointer -= 2;
        }
    }

  /* Skip any remaining whitespace */
  while ((my_file_pointer > 0) && (is_whitespace(file_buffer[my_file_pointer])))
    {
      my_file_pointer--;
    }

  end = my_file_pointer;
  while ((my_file_pointer > 0))
    {
      if (is_end_of_name(file_buffer[my_file_pointer]))
        {
          len = end - my_file_pointer;
          strncpy(name, &file_buffer[my_file_pointer + 1], len);
          name[len] = 0;
          return 1;
        }
      my_file_pointer--;
    }

  return 0;
}

static int
skip_to_next_name(char *name)
{
  while ((file_pointer < file_size))
    {
      if (file_buffer[file_pointer] == '(')
        {
          return get_previous_identifier(file_pointer - 1, name);
        }
      file_pointer++;
    }
  return 0;
}

// Build a path and filename so it is of the format [cvs-module][directory][filename].
// Also convert all backslashes into forward slashes.
static void
get_filename(char *cvspath, char *filename, char *result)
{
  strcpy(result, cvspath);
  strcat(result, filename);
  path_to_url(result);
}

static void
parse_file(char *fullname, char *cvspath, char *filename)
{
  PAPI_INFO api_info;
  char prev[200];
  char name[200];
  int tag_id;

  read_file(fullname);

  prev[0] = 0;
  do
    {
      tag_id = skip_to_next_tag();
      if (tag_id == TAG_UNKNOWN)
        {
          break;
        }

      /* Skip rest of the comments between the tag and the function name */
      skip_comments();

      if (skip_to_next_name(name))
        {
          if (strlen(name) == 0)
            {
              printf("Warning: empty function name in file %s. Previous function name was %s.\n",
                fullname, prev);
            }
          api_info = malloc(sizeof(API_INFO));
          if (api_info == NULL)
            {
              printf("Out of memory\n");
              exit(1);
            }

          api_info->tag_id = tag_id;
          strcpy(api_info->name, name);

          get_filename(cvspath, filename, api_info->filename);

          api_info->next = api_info_list;
          api_info_list = api_info;
          strcpy(prev, name);
        }
    } while (1);

  close_file();
}

#ifdef WIN32

/* Win32 version */
static void
process_directory (char *path, char *cvspath)
{
  struct _finddata_t f;
  int findhandle;
  char searchbuf[MAX_PATH];
  char buf[MAX_PATH];
  char newcvspath[MAX_PATH];

  strcpy(searchbuf, path);
  strcat(searchbuf, "*.*");

  findhandle =_findfirst(searchbuf, &f);
  if (findhandle != -1)
    {
      do
      	{
          if (f.attrib & _A_SUBDIR)
      	    {
              if (f.name[0] != '.')
                {
                  strcpy(buf, path);
                  strcat(buf, f.name);
                  strcat(buf, DIR_SEPARATOR_STRING);

                  strcpy(newcvspath, cvspath);
                  strcat(newcvspath, f.name);
                  strcat(newcvspath, "/");

                  process_directory(buf, newcvspath);
                }
              continue;
      	    }

          strcpy(buf, path);
          strcat(buf, f.name);

          /* Must be a .c file */
          if (!is_valid_file(buf))
            {
              continue;
            }

          parse_file(buf, cvspath, f.name);
      	}
      while (_findnext(findhandle, &f) == 0);
      _findclose(findhandle);
    }
  else
    {
      printf("Cannot open directory '%s'", path);
      exit(1);
    }
}

#else

/* Linux version */
static void
process_directory (char *path, char *cvspath)
{
  DIR *dirp;
  struct dirent *entry;
  struct stat stbuf;
  char buf[MAX_PATH];
  char newcvspath[MAX_PATH];

#ifdef HAVE_D_TYPE
  dirp = opendir(path);
  if (dirp != NULL)
    {
      while ((entry = readdir(dirp)) != NULL)
        {
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // skip self and parent

      	  if (entry->d_type == DT_REG) // normal file
      	    {
              // Check for an absolute path
              if (path[0] == DIR_SEPARATOR_CHAR)
                {
                  strcpy(buf, path);
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, entry->d_name);
                }
              else
                {
                  getcwd(buf, sizeof(buf));
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, path);
                  strcat(buf, entry->d_name);
                }

      	      if (stat(buf, &stbuf) == -1)
                {
                  printf("Can't access '%s' (%s)\n", buf, strerror(errno));
                  return;
                }

              if (S_ISDIR(stbuf.st_mode))
          	    {
                  strcpy(newcvspath, cvspath);
                  strcat(newcvspath, f.name);
                  strcat(newcvspath, "/");

                  process_directory(buf, newcvspath);
                  continue;
          	    }

              /* Must be a .c file */
              if (!is_valid_file(buf))
                {
                  continue;
                }
  
              parse_file(buf, cvspath, entry->d_name);
           }
      }
      closedir(dirp);
    }
  else
    {
      printf("Can't open %s\n", path);
      exit(1);
    }

#else

  dirp = opendir(path);
  if (dirp != NULL)
    {
      while ((entry = readdir(dirp)) != NULL)
      	{
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // skip self and parent

          // Check for an absolute path
          if (path[0] == DIR_SEPARATOR_CHAR)
            {
              strcpy(buf, path);
              strcat(buf, DIR_SEPARATOR_STRING);
              strcat(buf, entry->d_name);
            }
          else
            {
              getcwd(buf, sizeof(buf));
              strcat(buf, DIR_SEPARATOR_STRING);
              strcat(buf, path);
              strcat(buf, entry->d_name);
            }

          if (stat(buf, &stbuf) == -1)
            {
              printf("Can't access '%s' (%s)\n", buf, strerror(errno));
              return;
            }

          if (S_ISDIR(stbuf.st_mode))
      	    {
              strcpy(newcvspath, cvspath);
              strcat(newcvspath, entry->d_name);
              strcat(newcvspath, "/");

              process_directory(buf, newcvspath);
              continue;
      	    }

          /* Must be a .c file */
          if (!is_valid_file(buf))
            {
              continue;
            }

          parse_file(buf, cvspath, entry->d_name);
        }
      closedir(dirp);
    }
  else
    {
      printf("Can't open %s\n", path);
      exit(1);
    }

#endif
}

#endif

/*
 * This function compares two API entries. It returns a negative value if p is
 * before q, or a positive value if p is after q.
 */
static int
compare_api_order(PAPI_INFO p, PAPI_INFO q)
{
  return strcmp(p->name, q->name);
}

static void
generate_xml_for_component(char *component_name)
{
  PAPI_INFO api_info;
  char buf[200];
  int complete;
  int total;
  int implemented_total;
  int unimplemented_total;

  // Sort list
  api_info_list = sort_linked_list(api_info_list, 0, compare_api_order);

  implemented_total = 0;
  unimplemented_total = 0;

  api_info = api_info_list;
  while (api_info != NULL)
    {
      if (api_info->tag_id == TAG_IMPLEMENTED)
        {
          implemented_total ++;
        }
      else if (api_info->tag_id == TAG_UNIMPLEMENTED)
        {
          unimplemented_total ++;
        }

      api_info = api_info->next;
    }

  if (implemented_total + unimplemented_total > 0)
    {
      complete = ((implemented_total) * 100) / (implemented_total + unimplemented_total);
    }
  else
    {
      complete = 100;
    }

  sprintf(buf, "<component name=\"%s\" complete=\"%d\" implemented_total=\"%d\" unimplemented_total=\"%d\">",
    component_name, complete, implemented_total, unimplemented_total);
  write_line(buf);

  if (api_info_list != NULL)
    {
      write_line("<functions>");

      api_info = api_info_list;
      while (api_info != NULL)
        {
          sprintf(buf, "<function name=\"%s\" implemented=\"%s\" file=\"%s\">",
            api_info->name,
            api_info->tag_id == TAG_IMPLEMENTED ? "true" : "false",
            api_info->filename);
          write_line(buf);
          write_line("</function>");
          api_info = api_info->next;
        }

      write_line("</functions>");
    }

  write_line("</component>");
}

static void
read_input_file(char *input_file)
{
  char component_name[MAX_PATH];
  char component_path[MAX_PATH];
  char *canonical_path;
  unsigned int index;
  unsigned int start;
  PAPI_INFO api_info;
  PAPI_INFO next_api_info;
  char *buffer;
  int size;
  int len;

  in = fopen(input_file, "rb");
  if (in == NULL)
    {
    	printf("Cannot open input file");
    	exit(1);
    }

  // Get the size of the file
  fseek(in, 0, SEEK_END);
  size = ftell(in);

  // Load it all into memory
  buffer = malloc(size);
  if (buffer == NULL)
    {
      fclose(in);
      printf("Out of memory\n");
      exit(1);
    }
  fseek(in, 0, SEEK_SET);
  if (fread (buffer, 1, size, in) < 1)
    {
      fclose(in);
      printf("Read error in file %s\n", input_file);
      exit(1);
    }

  index = 0;

  write_line("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>");
  write_line("");
  write_line("<components>");

  while (1)
    {
      /* Free previous list */
      for (api_info = api_info_list; api_info != NULL; api_info = next_api_info)
        {
          next_api_info = api_info->next;
          free(api_info);
        }
      api_info_list = NULL;

      /* Skip whitespace and eol characters */
      while ((index < size) && (is_whitespace(buffer[index]) || (is_eol_char(buffer[index]))))
        {
          index++;
        }
      if ((file_pointer < size) && (buffer[index] == '\n'))
        {
          index++;
        }

      if (buffer[index] == ';')
        {
          /* Skip comments */
          while ((index < size) && (!is_eol_char(buffer[index])))
            {
              index++;
            }
          if ((index < size) && (buffer[index] == '\n'))
            {
              index++;
            }
          continue;
        }

      /* Get component name */
      start = index;
      while ((index < size) && (!is_whitespace(buffer[index])))
        {
          index++;
        }
      if (index >= size)
        {
          break;
        }

      len = index - start;
      strncpy(component_name, &buffer[start], len);
      component_name[len] = 0;

      /* Skip whitespace */
      while ((index < size) && (is_whitespace(buffer[index])))
        {
          index++;
        }
      if (index >= size)
        {
          break;
        }

      /* Get component path */
      start = index;
      while ((index < size) && (!is_whitespace(buffer[index]) && !is_eol_char(buffer[index])))
        {
          index++;
        }

      len = index - start;
      strncpy(component_path, &buffer[start], len);
      component_path[len] = 0;

      /* Append directory separator if needed */
      if (component_path[strlen(component_path)] != DIR_SEPARATOR_CHAR)
        {
          int i = strlen(component_path);
          component_path[strlen(component_path)] = DIR_SEPARATOR_CHAR;
          component_path[i + 1] = 0;
        }

      /* Skip to end of line */
      while ((index < size) && (!is_eol_char(buffer[index])))
        {
          index++;
        }
      if ((index < size) && (buffer[index] == '\n'))
        {
          index++;
        }

      canonical_path = convert_path(component_path);
      if (canonical_path != NULL)
        {
          process_directory(canonical_path, canonical_path);
          free(canonical_path);
          generate_xml_for_component(component_name);
        }
    }

  write_line("</components>");
}

static char HELP[] =
  "RGENSTAT  input-filename output-filename\n"
  "\n"
  "  input-filename   File containing list of components to process\n"
  "  output-filename  File to create\n";

int main(int argc, char **argv)
{
  char *input_file;
  char *output_file;
  int i;

  if (argc < 2)
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

  output_file = convert_path(argv[2]);
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

  read_input_file(input_file);

  fclose(out);

  return 0;
}

/* EOF */
