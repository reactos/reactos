#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifdef WIN32
#include <sys/utime.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

#if defined(WIN32)
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#define DOS_PATHS
#else
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#define UNIX_PATHS
#endif	

char* convert_path(char* origpath)
{
   char* newpath;
   int i;
   int length;
   
   //newpath = strdup(origpath);
	 newpath=malloc(strlen(origpath)+1);
	 strcpy(newpath,origpath);
   
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

   length = strlen(newpath);
   if (length > 0)
     {
        if (newpath[length - 1] == DIR_SEPARATOR_CHAR)
          newpath[length - 1] = 0;
     }
   return(newpath);
}

#define TRANSFER_SIZE      (65536)

int mkdir_p(char* path)
{
   if (chdir(path) == 0)
     {
	return(0);
     }
#ifdef UNIX_PATHS
   if (mkdir(path, 0755) != 0)
     {
	perror("Failed to create directory");
	exit(1);
     }
#else
   if (mkdir(path) != 0)
     {
	perror("Failed to create directory");
	exit(1);
     }
#endif
   
   if (chdir(path) != 0)
     {
	perror("Failed to change directory");
	exit(1);
     }
   return(0);
}

int
write_created_file()
{
   char filename[256];
   int id;
#ifdef WIN32
   time_t now;
   struct utimbuf fnow;
#endif

   strcpy(filename, ".created");

  id = open(filename, S_IWRITE, S_IRUSR | S_IWUSR);
  if (id < 0)
    {
      id = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
      if (id < 0)
        {
          fprintf(stderr, "Cannot create file %s.\n", filename);
          return(1);
        }
    }

  close(id);

#ifdef WIN32
  now = time(NULL);
  fnow.actime = now;
  fnow.modtime = now;
  (int) utime(filename, &fnow);
#else
  (int) utimes(filename, NULL);
#endif
   
   return 0;
}

int main(int argc, char* argv[])
{
   char* path1;
   char* csec;
   char buf[256];
   
   if (argc != 2)
     {
	fprintf(stderr, "Too many arguments\n");
	exit(1);
     }
   
   path1 = convert_path(argv[1]);
   
   if (isalpha(path1[0]) && path1[1] == ':' && path1[2] == DIR_SEPARATOR_CHAR)
     {
	csec = strtok(path1, DIR_SEPARATOR_STRING);
  sprintf(buf, "%s\\", csec);
	chdir(buf);
	csec = strtok(NULL, DIR_SEPARATOR_STRING);
     }
   else if (path1[0] == DIR_SEPARATOR_CHAR)
     {
	chdir(DIR_SEPARATOR_STRING);
	csec = strtok(path1, DIR_SEPARATOR_STRING);
     }
   else
     {
	csec = strtok(path1, DIR_SEPARATOR_STRING);
     }
   
   while (csec != NULL)
     {
	if (mkdir_p(csec) > 0)
    exit(1);
	csec = strtok(NULL, DIR_SEPARATOR_STRING);
     }

   exit(write_created_file());
}
