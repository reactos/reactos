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

  if (argc == 1)
    {
      prefix = "";
    }
  else
    {
      prefix = strdup(argv[1]);
    }

  i = 0;
  while ((ch = fgetc(stdin)) != ':')
    {
      buf[i] = ch;
      i++;
    }
  buf[i] = 0;
  
  dot = strrchr(buf, '.');
  if (dot != NULL)
    {
      *dot = 0;
    }
  fprintf(stdout, "%s/.%s.d %s/%s.o:", prefix, buf, prefix,buf);

  while ((ch = fgetc(stdin)) != EOF)
    {
      fputc(ch, stdout);
    }

  return(0);
}
