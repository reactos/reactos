/*
 * Copy a text file with end-of-line character transformation (EOL)
 *
 * Usage: rline input-file output-file
 */
#include <sys/stat.h>
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

int
fsize (FILE * f)
{
  struct stat st;
  int fh = fileno (f);
   
  if (fh < 0 || fstat (fh, &st) < 0)
    return -1;
  return (int) st.st_size;
}

int main(int argc, char* argv[])
{
  char* path1;
  char* path2;
  FILE* in;
  FILE* out;
  char* in_buf;
  char* out_buf;
  int in_size;
  int in_ptr;
  int linelen;
  int n_in;
  int n_out;
  char eol_buf[2];

  /* Terminate the line with windows EOL characters (CRLF) */
  eol_buf[0] = '\r';
  eol_buf[1] = '\n';

   if (argc != 3)
     {
	fprintf(stderr, "Wrong argument count\n");
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

  in_size = fsize(in);
  in_buf = malloc(in_size);
  if (in_buf == NULL)
    {
	  perror("Not enough free memory");
	  fclose(in);
	  exit(1);
    }

   out = fopen(path2, "wb");
   if (out == NULL)
     {
	perror("Cannot open output file");
	fclose(in);
	exit(1);
     }

  /* Read it all in */
  n_in = fread(in_buf, 1, in_size, in);

  in_ptr = 0;
  while (in_ptr < in_size)
    {
      linelen = 0;

      while ((in_ptr + linelen < in_size) && (in_buf[in_ptr + linelen] != '\r') && (in_buf[in_ptr + linelen] != '\n'))
        {
          linelen++;
        }
      if (linelen > 0)
        {
          n_out = fwrite(&in_buf[in_ptr], 1, linelen, out);
          in_ptr += linelen;
        }
      /* Terminate the line  */
      n_out = fwrite(&eol_buf[0], 1, sizeof(eol_buf), out);

      if ((in_ptr < in_size) && (in_buf[in_ptr] == '\r'))
        {
          in_ptr++;
        }

      if ((in_ptr < in_size) && (in_buf[in_ptr] == '\n'))
        {
          in_ptr++;
        }
    }

  free(in_buf);
  fclose(in);

  exit(0);
}
