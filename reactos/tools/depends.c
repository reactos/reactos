#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
  char buf[512];
  char buf2[512];
  char ch;
  unsigned int i, j;
  char* dot;
  char* ext;
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
  while ((ch = fgetc(stdin)) == '#')
    {
      while ((ch = fgetc(stdin)) != '\n' && ch != EOF)
        {
        }
    }
  if (ch != EOF)
    {
      buf[i] = ch;
      i++;
    }

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
  i = 0;
  while ((ch = fgetc(stdin)) == ' ' && ch != EOF)
    {
      buf2[i] = ch;
      i++;
    }
  if (i == 0)
    {
      return 0;
    }
  if (ch != EOF)
    {
      buf2[i] = ch;
      i++;
    }
  j = i;
  while ((ch = fgetc(stdin)) != ' ' && ch != EOF)
    {
      buf2[j] = ch;
      j++;
    }
  buf2[j] = 0;
  if (i == j)
    {
      return 0;
    }

  ext = strrchr(buf2, '.');
  if (ext != NULL)
    {
      if (0 == strcmp(ext, ".h"))
        {
	  ext = "h.gch";
	}
      else
        {
	  ext = NULL;
	}
    }

  dot = strrchr(buf, '.');
  if (dot != NULL)
    {
      *dot = 0;
    }
  fprintf(out, "%s/.%s.TAG %s/.%s.d %s/%s.%s:%s ", prefix, buf, prefix, buf, 
          prefix,buf,ext ? ext : "o" , buf2);

  while ((ch = fgetc(stdin)) != EOF)
    {
      fputc(ch, out);
    }

  return(0);
}
