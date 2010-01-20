/*
 * Convert debug output from running the regression tests
 * on ReactOS to an xml document.
 * Casper S. Hornstrup <chorns@users.sourceforge.net>
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <io.h>
#include <dos.h>
#else
#include <sys/io.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include <ctype.h>
#ifndef WIN32
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif

typedef struct _TEST_RESULT_INFO
{
  struct _TEST_RESULT_INFO *next;
  char testname[100];
  char result[200];
  int succeeded; /* 0 = failed, 1 = succeeded */
} TEST_RESULT_INFO, *PTEST_RESULT_INFO;


static FILE *out;
static FILE *file_handle = NULL;
static char *file_buffer = NULL;
static unsigned int file_size = 0;
static int file_pointer = 0;
static PTEST_RESULT_INFO test_result_info_list = NULL;


static char*
convert_path(char* origpath)
{
   char* newpath;
   int i;

   newpath = strdup(origpath);

   i = 0;
   while (newpath[i] != 0)
     {
#ifndef WIN32
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef WIN32
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

static void
write_line(char *line)
{
  int n_out;
  char buf[200];

  memset(buf, 0, sizeof(buf));
  strcpy(buf, line);
  /* Terminate the line */
  buf[strlen(buf)] = '\r';
  buf[strlen(buf)] = '\n';

  n_out = fwrite(&buf[0], 1, strlen(buf), out);
}


static void
read_file(char *filename)
{
  file_handle = fopen(filename, "rb");
  if (file_handle == NULL)
    {
      printf("Can't open %s\n", filename);
      exit(1);
    }

  // Get the size of the file
  fseek(file_handle, 0, SEEK_END);
  file_size = ftell(file_handle);

  // Load it all into memory
  file_buffer = malloc(file_size);
  if (file_buffer == NULL)
    {
      fclose(file_handle);
      printf("Out of memory\n");
      exit(1);
    }
  fseek(file_handle, 0, SEEK_SET);
  if (file_size > 0)
    {
      if (fread (file_buffer, 1, file_size, file_handle) < 1)
        {
          fclose(file_handle);
          printf("Read error in file %s\n", filename);
          exit(1);
        }
    }

  file_pointer = 0;
}

static void
close_file()
{
  free(file_buffer);
  file_buffer = NULL;
  fclose(file_handle);
  file_handle = NULL;
  file_pointer = 0;
}

static int
is_whitespace(char ch)
{
  if (ch == ' ')
    {
      return 1;
    }
  if (ch == '\t')
    {
      return 1;
    }
  return 0;
}

static int
is_eol_char(char ch)
{
  if (ch == '\r')
    {
      return 1;
    }
  if (ch == '\n')
    {
      return 1;
    }
  return 0;
}

static void
skip_line()
{
  while ((file_pointer < file_size) && (!is_eol_char(file_buffer[file_pointer])))
    {
      file_pointer++;
    }
  if ((file_pointer < file_size) && (is_eol_char(file_buffer[file_pointer])))
    {
      file_pointer++;
      if ((file_pointer < file_size) && (file_buffer[file_pointer] == '\n'))
        {
          file_pointer++;
        }
    }
}

static void
skip_whitespace()
{
  while ((file_pointer < file_size) && !is_eol_char(file_buffer[file_pointer])
    && is_whitespace(file_buffer[file_pointer]))
    {
      file_pointer++;
    }
}

static int
skip_to_next_test()
{
  static char test_marker[] = "ROSREGTEST:";
  int found_test = 0;

  while ((file_pointer < file_size) && (!found_test))
    {
	  skip_whitespace();
      found_test = 1;
	  int i = 0;
      while (1)
	    {
		  if (i >= strlen(test_marker))
            {
              break;
            }
		  if (is_eol_char(file_buffer[file_pointer]))
		    {
              found_test = 0;
			  break;
		    }
		  if (file_buffer[file_pointer] != test_marker[i])
            {
              found_test = 0;
			  break;
            }
          file_pointer++;
          i++;
	    }
	  if (!found_test)
        {
          skip_line();
        }
    }
  return found_test;
}

static int
read_until(char ch, char* buf)
{
  int start = file_pointer;
  while ((file_pointer < file_size))
    {
      if (file_buffer[file_pointer] == ch)
        {
		  strncpy(buf, &file_buffer[start], file_pointer - start);
		  buf[file_pointer - start] = 0;
          return 1;
        }
      file_pointer++;
    }
  return 0;
}

static int
read_until_end(char* buf)
{
  int start = file_pointer;
  while ((file_pointer < file_size))
    {
      if (is_eol_char(file_buffer[file_pointer]))
        {
		  strncpy(buf, &file_buffer[start], file_pointer - start);
		  buf[file_pointer - start] = 0;
		  skip_line();
          return 1;
        }
      file_pointer++;
    }
  return 0;
}

static void
parse_file(char *filename)
{
  PTEST_RESULT_INFO test_result_info;

  read_file(filename);

  do
    {
      if (!skip_to_next_test())
        {
          break;
        }

	  /*
	   * FORMAT:
	   * [ROSREGTEST:][space][|][<testname>][|][space][Status:][space][<result of running test>]
	   */

      test_result_info = malloc(sizeof(TEST_RESULT_INFO));
      if (test_result_info == NULL)
        {
	      printf("Out of memory\n");
	      exit(1);
        }

      /* Skip whitespaces */
      skip_whitespace();

  	  /* [|] */
      file_pointer++;

	  /* <testname> */
	  read_until(')', test_result_info->testname);

   	  /* [|] */
      file_pointer++;

	  /* [space] */
      file_pointer++;

  	  /* Status: */
      file_pointer += 7;

  	  /* [space] */
      file_pointer++;

  	  /* <result of running test> */
	  read_until_end(test_result_info->result);

	  if (strncmp(test_result_info->result, "Success", 7) == 0)
        {
	      test_result_info->succeeded = 1;
        }
      else
        {
          test_result_info->succeeded = 0;
        }

      test_result_info->next = test_result_info_list;
      test_result_info_list = test_result_info;
    } while (1);

  close_file();
}

static void
generate_xml()
{
  PTEST_RESULT_INFO test_result_info;
  char buf[200];
  int success_rate;
  int succeeded_total;
  int failed_total;

  succeeded_total = 0;
  failed_total = 0;

  test_result_info = test_result_info_list;
  while (test_result_info != NULL)
    {
      if (test_result_info->succeeded)
        {
          succeeded_total++;
        }
      else
        {
          failed_total++;
        }
      test_result_info = test_result_info->next;
    }

  if (succeeded_total + failed_total > 0)
    {
      success_rate = ((succeeded_total) * 100) / (succeeded_total + failed_total);
    }
  else
    {
      success_rate = 100;
    }

  write_line("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>");
  write_line("");

  sprintf(buf, "<testresults success_rate=\"%d\" succeeded_total=\"%d\" failed_total=\"%d\">",
    success_rate, succeeded_total, failed_total);
  write_line(buf);

  if (test_result_info_list != NULL)
    {
      test_result_info = test_result_info_list;
      while (test_result_info != NULL)
        {
          sprintf(buf, "<testresult testname=\"%s\" succeeded=\"%s\" result=\"%s\">",
            test_result_info->testname,
            test_result_info->succeeded == 1 ? "true" : "false",
            test_result_info->result);
          write_line(buf);
          write_line("</testresult>");
          test_result_info = test_result_info->next;
        }
    }

  write_line("</testresults>");
}

static char HELP[] =
  "REGTESTS2XML input-filename output-filename\n"
  "\n"
  "  input-filename   File containing output from running regression tests\n"
  "  output-filename  File to create\n";

int main(int argc, char **argv)
{
  char *input_file;
  char *output_file;

  if (argc < 3)
    {
      puts(HELP);
      return 1;
    }

  input_file = convert_path(argv[1]);
  if (input_file[0] == 0)
    {
      printf("Missing input-filename\n");
      return 1;
    }

  output_file = convert_path(argv[2]);
  if (output_file[0] == 0)
    {
      printf("Missing output-filename\n");
      return 1;
    }

  out = fopen(output_file, "wb");
  if (out == NULL)
    {
    	printf("Cannot open output file");
    	return 1;
     }

  parse_file(input_file);

  generate_xml();

  fclose(out);

  return 0;
}
