#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

int
write_if_change(char* outbuf, char* filename)
{
  FILE* out;
  unsigned int end;
  char* cmpbuf;
  unsigned int stat;

  out = fopen(filename, "rb");
  if (out == NULL)
    {
      out = fopen(filename, "wb");
      if (out == NULL)
	{
	  fprintf(stderr, "Unable to create output file\n");
	  return(1);
	}
      fputs(outbuf, out);
      fclose(out);
      return(0);
    }

  fseek(out, 0, SEEK_END);
  end = ftell(out);
  cmpbuf = malloc(end);
  if (cmpbuf == NULL)
    {
      fprintf(stderr, "Out of memory\n");
      fclose(out);
      return(1);
    }

  fseek(out, 0, SEEK_SET);
  stat = fread(cmpbuf, 1, end, out);
  if (stat != end)
    {
      fprintf(stderr, "Failed to read data\n");
      fclose(out);
      return(1);
    }
  if (memcmp(cmpbuf, outbuf, max(end, strlen(outbuf))) == 0)
    {
      fclose(out);
      return(0);
    }

  fclose(out);
  out = fopen(filename, "wb");
  if (out == NULL)
    {
      fprintf(stderr, "Unable to create output file\n");
      return(1);
    }

  stat = fwrite(outbuf, 1, strlen(outbuf), out);
  if (strlen(outbuf) != stat)
    {
      fprintf(stderr, "Unable to write output file\n");
      fclose(out);
      return(1);
    }
  fclose(out);
  return(0);
}

int
main(int argc, char* argv[])
{
  unsigned int i;
  char* outbuf;
  char* s;

  if (argc == 1)
    {
      fprintf(stderr, "Not enough arguments\n");
      return(1);
    }

  outbuf = malloc(256 * 1024);
  if (outbuf == NULL)
    {
      fprintf(stderr, "Out of memory 1\n");
      return(1);
    }

  s = outbuf;
  s = s + sprintf(s, "/* Automatically generated, don't edit */\n");
  for (i = 2; i < argc; i++)
    {
      s = s + sprintf(s, "#define %s\n", argv[i]);
    }

  return(write_if_change(outbuf, argv[1]));
}
