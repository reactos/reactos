/*
 * Generate a file with test registrations from a list
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

static FILE *out;
static char *path;
static char *file;

char* convert_path(char* origpath)
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

static void write_line(char *line)
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

void register_test(char *filename, int prototype)
{
  char ext[100];
  char testname[100];
  char call[100];
  char regtest[100];
  int i, j;

  strcpy(testname, filename);

  i = strlen(testname);
  while (i > 0 && testname[i] != '.')
    {
      i--;
    }
  if (i > 0)
    {
      memset(ext, 0, sizeof(ext));
      strncpy(&ext[0], &testname[i], strlen(&testname[i]));

      if ((strncmp(ext, ".c", 2) != 0) && (strncmp(ext, ".C", 2) != 0))
        {
          return;
        }

      testname[i] = 0;
    }
  else
    {
      return;
    }

  // Make a capital first letter and make all other letters lower case
  testname[0] = toupper(testname[0]);
  if (!((testname[0] >= 'A' && testname[0] <= 'Z') ||
    (testname[0] >= '0' && testname[0] <= '9')))
    {
      testname[0] = '_';
    }
  j = 1;
  while (j < strlen(testname))
    {
      testname[j] = tolower(testname[j]);
      if (!((testname[j] >= 'a' && testname[j] <= 'z') ||
        (testname[j] >= '0' && testname[j] <= '9')))
        {
          testname[j] = '_';
        }
      j++;
    }

  if (prototype)
    {
      sprintf(regtest, "extern int %sTest(int Command, char *Buffer);", testname);
      write_line(regtest);
    }
  else
    {
      sprintf(call, "%sTest", testname);
      sprintf(regtest, "  AddTest((TestRoutine)%s);", call);
      write_line(regtest);
    }
}

#ifdef WIN32

/* Win32 version */

static void
make_file_list (int prototype)
{
  struct _finddata_t f;
  int findhandle;
  char searchbuf[MAX_PATH];

  strcpy(searchbuf, path);
  strcpy(searchbuf, "*.*");
  findhandle =_findfirst(searchbuf, &f);
  if (findhandle != -1)
    {
      do
      	{
      	  if (f.attrib & _A_SUBDIR)
      	    {
              /* Skip subdirectories */
              continue;
      	    }

          register_test(f.name, prototype);
      	}
      while (_findnext(findhandle, &f) == 0);
      _findclose(findhandle);
    }
}

#else

/* Linux version */
static void
make_file_list (int prototype)
{
  DIR *dirp;
  struct dirent *entry;
  struct stat stbuf;
  char buf[MAX_PATH];

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
              /* Skip subdirectories */
              continue;
      	    }

          register_test(entry->d_name, prototype);
         }
      }
      closedir(dirp);
    }
  else
    {
      printf("Can't open %s\n", path);
      return;
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
              /* Skip subdirectories */
              continue;
      	    }

          register_test(entry->d_name, prototype);
        }
      closedir(dirp);
    }
  else
    {
      printf("Can't open %s\n", path);
      return;
    }

#endif
}

#endif

static char HELP[] =
  "REGTESTS  path file\n"
  "\n"
  "  path          Path to files\n"
  "  file          File to create\n";

int main(int argc, char **argv)
{
  char buf[MAX_PATH];
  int i;

  if (argc < 2)
  {
    puts(HELP);
    return 1;
  }


  strcpy(buf, convert_path(argv[1]));
  if (buf[strlen(buf)] != DIR_SEPARATOR_CHAR)
    {
      int i = strlen(buf);
      buf[strlen(buf)] = DIR_SEPARATOR_CHAR;
      buf[i + 1] = 0;
    }
  path = buf;
  if (path[0] == 0)
    {
      printf("Missing path\n");
      return 1;
    }

  file = convert_path(argv[2]);
  if (file[0] == 0)
    {
      printf("Missing file\n");
      return 1;
    }

  out = fopen(file, "wb");
  if (out == NULL)
    {
    	perror("Cannot open output file");
    	return 1;
     }

  write_line("/* This file is autogenerated. */");
  write_line("");
  write_line("typedef int (*TestRoutine)(int Command, char *Buffer);");
  write_line("");

  make_file_list(1);

  write_line("");
  write_line("extern void AddTest(TestRoutine Routine);");
  write_line("");
  write_line("void RegisterTests()");
  write_line("{");

  make_file_list(0);

  write_line("}");

  fclose(out);

  return 0;
}

/* EOF */
