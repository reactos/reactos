#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
  char buf[512];
  char ch;
  unsigned int i;
  char* dot;
  char* prefix;
  FILE* out;

  if (argc != 3)
    {
      printf("Too few arguments\n");
      return(1);
    }

  prefix = strdup(argv[1]);
  
  out = fopen(argv[2], "wb");
  if (out == NULL)
    {
      printf("Unable to open output file\n");
      return(1);
    }

  i = 0;
  while ((ch = fgetc(stdin)) != ':' && ch != EOF)
    {
      buf[i] = ch;
      i++;
    }
  buf[i] = 0;
  
  if (i == 0)
    {
      return(0);
    }

  dot = strrchr(buf, '.');
  if (dot != NULL)
    {
      *dot = 0;
    }
  fprintf(out, "%s/.%s.TAG %s/.%s.d %s/%s.o:", prefix, buf, prefix, buf, 
	  prefix,buf);

  while ((ch = fgetc(stdin)) != EOF)
    {
      fputc(ch, out);
    }

  return(0);
}
