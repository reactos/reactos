#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#include <dos.h>
#else
#include <sys/io.h>
#include <errno.h>
#include <sys/types.h> 
#include <dirent.h>
#endif
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

char* convert_path(char* origpath)
{
   char* newpath;
   int i;
   
   newpath = strdup(origpath);
   
   i = 0;
   while (newpath[i] != 0)
     {
#ifdef UNIX_PATHS
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef DOS_PATHS
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

#define TRANSFER_SIZE      (65536)

static void
copy_file(char* path1, char* path2)
{
   FILE* in;
   FILE* out;
   char* buf;
   int n_in;
   int n_out;

   in = fopen(path1, "rb");
   if (in == NULL)
     {
	perror("Cannot open input file");
	exit(1);
     }

   out = fopen(path2, "wb");
   if (out == NULL)
     {
	perror("Cannot open output file");
	fclose(in);
	exit(1);
     }
   
   buf = malloc(TRANSFER_SIZE);
   while (!feof(in))
     {
	n_in = fread(buf, 1, TRANSFER_SIZE, in);
	n_out = fwrite(buf, 1, n_in, out);
	if (n_in != n_out)
	  {
	     perror("Failed to write to output file\n");
	     free(buf);
	     fclose(in);
	     fclose(out);
	     exit(1);
	  }
     }
}

#ifdef WIN32

static void
copy_directory (char *path1, char *path2)
{
  struct _finddata_t f;
  int findhandle;
  char buf[MAX_PATH];
  char tobuf[MAX_PATH];

  strcpy(buf, path1);
  if (path1[strlen(path1) - 1] != DIR_SEPARATOR_CHAR)
    strcat(buf, DIR_SEPARATOR_STRING);
  strcat(buf, "*.*");
  findhandle =_findfirst(buf, &f);
  if (findhandle != 0)
    {
      do
    	{
    	  if ((f.attrib & _A_SUBDIR) == 0 && f.name[0] != '.')
    	    {
              // Check for an absolute path
              if (path1[0] == DIR_SEPARATOR_CHAR)
                {
                  strcpy(buf, path1);
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, f.name);
                }
              else
                {
                  getcwd(buf, sizeof(buf));
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, path1);
                  if (path1[strlen(path1) - 1] != DIR_SEPARATOR_CHAR)
                    strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, f.name);
                }

    		      //printf("copying file %s\n", buf);
              if (path2[strlen(path2) - 1] == DIR_SEPARATOR_CHAR)
                {
                  strcpy(tobuf, path2);
                  strcat(tobuf, f.name);
                }
              else
                {
                  strcpy(tobuf, path2);
                  strcat(tobuf, DIR_SEPARATOR_STRING);
                  strcat(tobuf, f.name);
                }
              copy_file(buf, tobuf);
    	    }
        else
          {
            //printf("skipping directory '%s'\n", f.name);
          }
    	}
       while (_findnext(findhandle, &f) == 0);

      _findclose(findhandle);
    }
}

#else

/* Linux version */
static void
copy_directory (char *path1, char *path2)
{
  DIR *dirp;
  struct dirent *entry;
  char *old_end_source;
  struct stat stbuf;
  char buf[MAX_PATH];
  char tobuf[MAX_PATH];
  char err[400];

  dirp = opendir(path1); 

  if (dirp != NULL)
    {
      while ((entry = readdir (dirp)) != NULL)
	  {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue; // skip self and parent

        if (entry->d_type == DT_REG) // normal file
		    {
              // Check for an absolute path
              if (path1[0] == DIR_SEPARATOR_CHAR)
                {
                  strcpy(buf, path1);
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, entry->d_name);
                }
              else
                {
                  getcwd(buf, sizeof(buf));
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, path1);
                  if (path1[strlen(path1) - 1] != DIR_SEPARATOR_CHAR)
                    strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, entry->d_name);
                }
              if (stat(buf, &stbuf) == -1)
                {
                  sprintf(err, "Can't access '%s' (%s)\n", buf, strerror(errno));
                  perror(err);
                  exit(1);
                  return;
                }

			      //printf("copying file '%s'\n", entry->d_name);
            if (path2[strlen(path2) - 1] == DIR_SEPARATOR_CHAR)
              {
                strcpy(tobuf, path2);
                strcat(tobuf, entry->d_name);
              }
            else
              {
                strcpy(tobuf, path2);
                strcat(tobuf, DIR_SEPARATOR_STRING);
                strcat(tobuf, entry->d_name);
              }
            copy_file(buf, tobuf);
         }
        else
          {
            //printf("skipping directory '%s'\n", entry->d_name);
          }
       }
      closedir (dirp);
    }
  else
    {
      sprintf(err, "Can't open %s\n", path1);
      perror(err);
      exit(1);
      return;
    }
}

#endif

static int
is_directory(char *path)
{
  struct stat stbuf;
  char buf[MAX_PATH];
  char err[400];

  // Check for an absolute path
  if (path[0] == DIR_SEPARATOR_CHAR)
    {
      strcpy(buf, path);
    }
  else
    {
      getcwd(buf, sizeof(buf));
      strcat(buf, DIR_SEPARATOR_STRING);
      strcat(buf, path);
    }
  if (stat(buf, &stbuf) == -1)
    {
      /* Assume a destination file */
      return 0;
    }
  if (S_ISDIR(stbuf.st_mode))
    return 1;
  else
    return 0;
}

int main(int argc, char* argv[])
{
   char* path1;
   char* path2;
   int dir1;
   int dir2;
   
   if (argc != 3)
     {
	fprintf(stderr, "Wrong argument count\n");
	exit(1);
     }

   path1 = convert_path(argv[1]);
   path2 = convert_path(argv[2]);

   dir1 = is_directory(path1);
   dir2 = is_directory(path2);

   if ((dir1 && !dir2) || (!dir1 && dir2))
    {
	     perror("None or both paramters must be a directory\n");
	     exit(1);
    }

   if (dir1)
     {
       copy_directory(path1, path2);
     }
   else
     {
       copy_file(path1, path2);
     }

   exit(0);
}
