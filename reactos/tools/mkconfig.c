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
  if (end == strlen(outbuf) && memcmp(cmpbuf, outbuf, end) == 0)
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
  char config[512];

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
  s = s + sprintf(s, "/* Automatically generated, ");
  s = s + sprintf(s, "Edit the Makefile to change configuration */\n");
  s = s + sprintf(s, "#ifndef __NTOSKRNL_INCLUDE_INTERNAL_CONFIG_H\n");
  s = s + sprintf(s, "#define __NTOSKRNL_INCLUDE_INTERNAL_CONFIG_H\n");
  strcpy(config, "");
  for (i = 2; i < argc; i++)
    {
      s = s + sprintf(s, "#define %s 1\n", argv[i]);
      strcat(config, argv[i]);
      if (i != (argc - 1))
	{
	  strcat(config, " ");
	}
    }
  s = s + sprintf(s, "#define CONFIG \"%s\"\n", config);
  s = s + sprintf(s, "#endif /* __NTOSKRNL_INCLUDE_INTERNAL_CONFIG_H */\n");

  return(write_if_change(outbuf, argv[1]));
}
