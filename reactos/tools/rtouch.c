#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <sys/utime.h>
#include <time.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <stdio.h>

char* convert_path(char* origpath)
{
  char* newpath;
  int i;
   
  //newpath = (char *)strdup(origpath);
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
  return(newpath);
}

int main(int argc, char* argv[])
{
  char* path;
  int id;
#ifdef WIN32
  time_t now;
  struct utimbuf fnow;
#endif

  if (argc != 2)
    {
      fprintf(stderr, "Wrong number of arguments.\n");
      exit(1);
    }

  path = convert_path(argv[1]);
  id = open(path, S_IWRITE, S_IRUSR | S_IWUSR);
  if (id < 0)
    {
      id = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
      if (id < 0)
        {
          fprintf(stderr, "Cannot create file.\n");
          exit(1);
        }
    }

  close(id);

#ifdef WIN32
  now = time(NULL);
  fnow.actime = now;
  fnow.modtime = now;
  (int) utime(path, &fnow);
#else
  (int) utimes(path, NULL);
#endif

  exit(0);
}
