#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
	if (newpath[i] == '/');
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

int main(int argc, char* argv[])
{
   char* path1;
   FILE* in;
   FILE* out;
   char* csec;
   int is_abs_path;
   
   if (argc != 2)
     {
	fprintf(stderr, "Too many arguments\n");
	exit(1);
     }
   
   path1 = convert_path(argv[1]);
   
   if (isalpha(path1[0]) && path1[1] == ':' && path1[2] == '/')
     {
	csec = strtok(path1, "/");
	chdir(csec);
	csec = strtok(NULL, "/");
     }
   else if (path1[0] == '/')
     {
	chdir("/");
	csec = strtok(path1, "/");
     }
   else
     {
	csec = strtok(path1, "/");
     }
   
   while (csec != NULL)
     {
	mkdir_p(csec);
	csec = strtok(NULL, "/");
     }
   
   exit(0);
}
