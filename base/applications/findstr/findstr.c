/* findstr.c */

/* Copyright (C) 1994-2002, Jim Hall <jhall@freedos.org> */

/* Adapted for ReactOS -Edited for Findstr.exe K'Williams */

/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


/* This program locates a string in a text file and prints those lines
 * that contain the string.  Multiple files are clearly separated.
 */

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
//#include <io.h>
#include <dos.h>

#include "resource.h"

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
          int count_lines, int number_output, int ignore_case, int at_start, int literal_search,
		  int at_end, int reg_express, int exact_match, int sub_dirs, int only_fname)
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
	TCHAR lpUsage[4096];

	LoadString( GetModuleHandle(NULL), IDS_USAGE, (LPTSTR)lpUsage, 4096);
	CharToOem(lpUsage, lpUsage);
	printf( lpUsage );
}


/* Main program */
int
main (int argc, char **argv)
{
  char *opt, *needle = NULL;
  int ret = 0;
  TCHAR lpMessage[4096];

  int invert_search = 0;		/* flag to invert the search */
  int count_lines = 0;			/* flag to whether/not count lines */
  int number_output = 0;		/* flag to print line numbers */
  int ignore_case = 0;			/* flag to be case insensitive */
  int at_start = 0;				/* flag to Match if at the beginning of a line. */
  int at_end = 0;	        	/* flag to Match if at the beginning of a line. */
  int reg_express = 0;		   /* flag to use/not use regular expressions */
  int exact_match = 0;			/* flag to be exact match */
  int sub_dirs= 0;				/* this and all subdirectories */
  int only_fname= 0;			/* print only the name of the file*/
  int literal_search=0;

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
	      case 'b':
	      case 'B':		/* Matches pattern if at the beginning of a line */
	        at_start = 1;
	        break;

	      //case 'c':
	      //case 'C':		/* Literal? */
	      //  literal_search = 1;
	      //  break;

	      case 'e':
	      case 'E':		/* matches pattern if at end of line */
	        at_end = 1;
	        break;

	      case 'i':
	      case 'I':		/* Ignore */
	        ignore_case = 1;
	        break;

	      case 'm':
	      case 'M':		/* only filename */
	        only_fname = 1;
	        break;

	      case 'n':
	      case 'N':		/* Number */
	        number_output = 1;
	        break;

	      case 'r':
	      case 'R':		/* search strings as regular expressions */
	        reg_express = 1;
	        break;

	      case 's':
	      case 'S':		/* search files in child directory too*/
	        sub_dirs = 1;
	        break;

	      case 'v':
	      case 'V':		/* Not with */
	        invert_search = 1;
	        break;

	      case 'x':
	      case 'X':		/* exact match */
	        exact_match = 1;
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
                      number_output, ignore_case, at_start, literal_search, at_end, reg_express, exact_match,
					  sub_dirs, only_fname);
    }

  while (--argc >= 0)
    {
      hfind = _findfirst (*++argv, &finddata);
      if (hfind < 0)
	{
	  /* We were not able to find a file. Display a message and
	     set the exit status. */
	  LoadString( GetModuleHandle(NULL), IDS_NO_SUCH_FILE, (LPTSTR)lpMessage, 4096);
	  CharToOem(lpMessage, lpMessage);
	  fprintf (stderr, lpMessage, *argv);//
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
                      number_output, ignore_case, at_start, literal_search, at_end, reg_express, exact_match,
					  sub_dirs, only_fname);
	          fclose (pfile);
	        }
 	      else
	        {
	          LoadString(GetModuleHandle(NULL), IDS_CANNOT_OPEN, (LPTSTR)lpMessage, 4096);
	          CharToOem(lpMessage, lpMessage);
	          fprintf (stderr, lpMessage,
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


