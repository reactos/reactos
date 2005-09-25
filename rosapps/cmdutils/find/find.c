/* find.c */

/* Copyright (C) 1994-2002, Jim Hall <jhall@freedos.org> */

/* Adapted for ReactOS */

/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/* This program locates a string in a text file and prints those lines
 * that contain the string.  Multiple files are clearly separated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <dir.h>
#include <dos.h>


/* Symbol definition */
#define MAX_STR 1024


/* This function prints out all lines containing a substring.  There are some
 * conditions that may be passed to the function.
 *
 * RETURN: If the string was found at least once, returns 1.
 * If the string was not found at all, returns 0.
 */
int
find_str (char *sz, FILE *p, int invert_search,
          int count_lines, int number_output, int ignore_case)
{
  int i, length;
  long line_number = 0, total_lines = 0;
  char *c, temp_str[MAX_STR], this_line[MAX_STR];

  /* Convert to upper if needed */
  if (ignore_case)
    {
      length = strlen (sz);
      for (i = 0; i < length; i++)
	sz[i] = toupper (sz[i]);
    }

  /* Scan the file until EOF */
  while (fgets (temp_str, MAX_STR, p) != NULL)
    {
      /* Remove the trailing newline */
      length = strlen (temp_str);
      if (temp_str[length-1] == '\n')
	{
	  temp_str[length-1] = '\0';
	}

      /* Increment number of lines */
      line_number++;
      strcpy (this_line, temp_str);

      /* Convert to upper if needed */
      if (ignore_case)
	{
	  for (i = 0; i < length; i++)
	    {
	      temp_str[i] = toupper (temp_str[i]);
	    }
	}

      /* Locate the substring */

      /* strstr() returns a pointer to the first occurrence in the
       string of the substring */
      c = strstr (temp_str, sz);

      if ( ((invert_search) ? (c == NULL) : (c != NULL)) )
	{
	  if (!count_lines)
	    {
	      if (number_output)
		printf ("%ld:", line_number);

	      /* Print the line of text */
	      puts (this_line);
	    }

	  total_lines++;
	} /* long if */
    } /* while fgets */

  if (count_lines)
    {
      /* Just show num. lines that contain the string */
      printf ("%ld\n", total_lines);
    }


 /* RETURN: If the string was found at least once, returns 1.
  * If the string was not found at all, returns 0.
  */
  return (total_lines > 0 ? 1 : 0);
}


/* Show usage */
void
usage (void)
{
  fprintf (stderr, "FIND: Prints all lines of a file that contain a string\n");
  fprintf (stderr, "FIND [ /C ] [ /I ] [ /N ] [ /V ] \"string\" [ file... ]\n");
  fprintf (stderr, "  /C  Count the number of lines that contain string\n");
  fprintf (stderr, "  /I  Ignore case\n");
  fprintf (stderr, "  /N  Number the displayed lines, starting at 1\n");
  fprintf (stderr, "  /V  Print lines that do not contain the string\n");
}


/* Main program */
int
main (int argc, char **argv)
{
  char *opt, *needle = NULL;
  int ret = 0;

  int invert_search = 0;		/* flag to invert the search */
  int count_lines = 0;			/* flag to whether/not count lines */
  int number_output = 0;		/* flag to print line numbers */
  int ignore_case = 0;			/* flag to be case insensitive */

  FILE *pfile;				/* file pointer */
  int hfind;				/* search handle */
  struct _finddata_t finddata;		/* _findfirst, filenext block */

  /* Scan the command line */
  while ((--argc) && (needle == NULL))
    {
      if (*(opt = *++argv) == '/')
        {
          switch (opt[1])
	    {
	      case 'c':
	      case 'C':		/* Count */
	        count_lines = 1;
	        break;

	      case 'i':
	      case 'I':		/* Ignore */
	        ignore_case = 1;
	        break;

	      case 'n':
	      case 'N':		/* Number */
	        number_output = 1;
	        break;

	      case 'v':
	      case 'V':		/* Not with */
	        invert_search = 1;
	        break;

	      default:
	        usage ();
	        exit (2);		/* syntax error .. return error 2 */
	        break;
	    }
        }
      else
        {
          /* Get the string */
	  if (needle == NULL)
	    {
              /* Assign the string to find */
              needle = *argv;
	    }
	}
    }

  /* Check for search string */
  if (needle == NULL)
    {
      /* No string? */
      usage ();
      exit (1);
    }

  /* Scan the files for the string */
  if (argc == 0)
    {
      ret = find_str (needle, stdin, invert_search, count_lines,
                      number_output, ignore_case);
    }

  while (--argc >= 0)
    {
      hfind = _findfirst (*++argv, &finddata);
      if (hfind < 0)
	{
	  /* We were not able to find a file. Display a message and
	     set the exit status. */
	  fprintf (stderr, "FIND: %s: No such file\n", *argv);
	}
      else
        {
          /* repeat find next file to match the filemask */
	  do
            {
              /* We have found a file, so try to open it */
	      if ((pfile = fopen (finddata.name, "r")) != NULL)
	        {
	          printf ("---------------- %s\n", finddata.name);
	          ret = find_str (needle, pfile, invert_search, count_lines,
	                          number_output, ignore_case);
	          fclose (pfile);
	        }
 	      else
	        {
	          fprintf (stderr, "FIND: %s: Cannot open file\n",
		           finddata.name);
                }
	    }
          while (_findnext(hfind, &finddata) > 0);
        }
      _findclose(hfind);
    } /* for each argv */

 /* RETURN: If the string was found at least once, returns 0.
  * If the string was not found at all, returns 1.
  * (Note that find_str.c returns the exact opposite values.)
  */
  exit ( (ret ? 0 : 1) );
}

