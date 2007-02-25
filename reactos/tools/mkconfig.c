#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

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
  int include_tests;
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
  s = s + sprintf(s, "#ifndef __INCLUDE_CONFIG_H\n");
  s = s + sprintf(s, "#define __INCLUDE_CONFIG_H\n");
  strcpy(config, "");
  include_tests = 0;
  for (i = 2; i < argc; i++)
    {
      if (strcmp(argv[i], "REGTESTS") == 0)
        {
          include_tests = 1;
        }
      else
        {
          s = s + sprintf(s, "#ifndef %s\n", argv[i]);
          s = s + sprintf(s, "#define %s\n", argv[i]);
          s = s + sprintf(s, "#endif /* %s */\n", argv[i]);
        }
      strcat(config, argv[i]);
      if (i != (argc - 1))
	{
	  strcat(config, " ");
	}
    }
  if (include_tests)
    {
      s = s + sprintf(s, "#ifndef __ASM__\n");
      s = s + sprintf(s, "extern void PrepareTests();\n");
      s = s + sprintf(s, "#define PREPARE_TESTS PrepareTests();\n");
	  s = s + sprintf(s, "#endif /* __ASM__ */\n");
    }
  else
    {
      s = s + sprintf(s, "#define PREPARE_TESTS\n");
    }
  s = s + sprintf(s, "#define CONFIG \"%s\"\n", config);
  s = s + sprintf(s, "#endif /* __INCLUDE_CONFIG_H */\n");

  return(write_if_change(outbuf, argv[1]));
}
