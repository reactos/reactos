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

int main(int argc, char* argv[])
{
   char* path1;
   char* path2;
   FILE* in;
   FILE* out;
   char* buf;
   int n_in;
   int n_out;
   
   if (argc != 3)
     {
	fprintf(stderr, "Too many arguments\n");
	exit(1);
     }
   
   path1 = convert_path(argv[1]);
   path2 = convert_path(argv[2]);
   
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
   exit(0);
}
