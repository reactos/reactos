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

static FILE *out;
static char *path;
static char *file;
static char *makefile;

char* convert_path(char* origpath)
{
   char* newpath;
   int i;
   
	 /* for no good reason, i'm having trouble getting gcc to link strdup */
   //newpath = strdup(origpath);
	 newpath = malloc(strlen(origpath)+1);
	 strcpy(newpath, origpath);
   
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

static void change_extension(char *filenamebuffer, char *filename, char *newextension)
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

/*
 * filename - name of file to make registrations for
 * regtype  - type of registration (0 = prototype, 1 = call, 2 = makefile)
 */
void register_test(char *filename, int type)
{
  char ext[100];
  char testname[100];
  char call[100];
  char regtest[100];
  char filenamebuffer[MAX_PATH];
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

  if (type == 0)
    {
      sprintf(regtest, "extern int %sTest(int Command, char *Buffer);", testname);
      write_line(regtest);
    }
  else if (type == 1)
    {
      sprintf(call, "%sTest", testname);
      sprintf(regtest, "  AddTest((TestRoutine)%s);", call);
      write_line(regtest);
    }
  else if (type == 2)
    {
	  change_extension(filenamebuffer, filename, ".o");
      sprintf(regtest, "%s \\", filenamebuffer);
      write_line(regtest);
    }
}

#ifdef WIN32

/* Win32 version */

static void
make_file_list (int type)
{
  struct _finddata_t f;
  int findhandle;
  char searchbuf[MAX_PATH];

  strcpy(searchbuf, path);
  strcat(searchbuf, "*.*");
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

          register_test(f.name, type);
      	}
      while (_findnext(findhandle, &f) == 0);
      _findclose(findhandle);
    }
}

#else

/* Linux version */
static void
make_file_list (int type)
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

          register_test(entry->d_name, type);
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

          register_test(entry->d_name, type);
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
  "REGTESTS path file makefile\n"
  "\n"
  "  path        Path to files\n"
  "  file        Registration file to create\n"
  "  makefile    Makefile to create\n";

int main(int argc, char **argv)
{
  char buf[MAX_PATH];
  if (argc < 4)
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

  makefile = convert_path(argv[3]);
  if (makefile[0] == 0)
    {
      printf("Missing makefile\n");
      return 1;
    }


  /* Registration file */
  out = fopen(file, "wb");
  if (out == NULL)
    {
    	perror("Cannot create output file");
    	return 1;
     }

  write_line("/* This file is autogenerated. */");
  write_line("");
  write_line("typedef int (*TestRoutine)(int Command, char *Buffer);");
  write_line("");

  make_file_list(0);

  write_line("");
  write_line("extern void AddTest(TestRoutine Routine);");
  write_line("");
  write_line("void RegisterTests()");
  write_line("{");

  make_file_list(1);

  write_line("}");

  fclose(out);


  /* Makefile */
  out = fopen(makefile, "wb");
  if (out == NULL)
    {
    	perror("Cannot create output makefile");
    	return 1;
     }

  write_line("# This file is autogenerated.");
  write_line("");
  write_line("TESTS = \\");

  make_file_list(2);

  write_line("");

  fclose(out);

  printf("Successfully generated regression test registrations.\n");

  return 0;
}
