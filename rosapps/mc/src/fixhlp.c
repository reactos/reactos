/* HLP converter
   Copyright (C) 1994, 1995 Janne Kukonlehto
   Copyright (C) 1995  Jakub Jelinek
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "help.h"

#define BUFFER_SIZE 256

static int width;		/* Output width in characters */
static int col = 0;		/* Current output column */
static FILE *toc_file;		/* TOC file */
static int out_row = 1;		/* Current output row */
static int in_row = 0;		/* Current input row */
static int indent = 1;
static int curindent = 1;
static int freshnl = 1;
static int firstlen = 0;
static int verbatim = 0;

/* Report error in input */
void print_error (char *message)
{
    fprintf (stderr, "fixhlp: %s at row %d\n", message, in_row);
}

/* Change output line */
void newline (void)
{
    out_row ++;
    col = indent;
    curindent = indent;
    printf("\n%*s", indent, "");
    freshnl = 1;
    firstlen = 0;
}

/* Calculate the length of string */
int string_len (char *buffer)
{
    static int anchor_flag = 0;	/* Flag: Inside hypertext anchor name */
    static int link_flag = 0;	/* Flag: Inside hypertext link target name */
    int i;			/* Index */
    int c;			/* Current character */
    int len = 0;		/* Result: the length of the string */

    for (i = 0; i < strlen (buffer); i ++)
    {
	c = buffer [i];
	if (c == CHAR_LINK_POINTER) 
	    link_flag = 1;	/* Link target name starts */
	else if (c == CHAR_LINK_END)
	    link_flag = 0;	/* Link target name ends */
	else if (c == CHAR_NODE_END){
	    /* Node anchor name starts */
	    anchor_flag = 1;
	    /* Ugly hack to prevent loss of one space */
	    len ++;
	}
	/* Don't add control characters to the length */
	if (c < 32)
	    continue;
	/* Attempt to handle backslash quoting */
	/* Increase length if not inside anchor name or link target name */
	if (!anchor_flag && !link_flag)
	    len ++;
	if (anchor_flag && c == ']'){
	    /* Node anchor name ends */
	    anchor_flag = 0;
	}
    }
    return len;
}

/* Output the string */
void print_string (char *buffer)
{
    int len;	/* The length of current word */
    int i;	/* Index */
    int c;	/* Current character */
    char *p;

    /* Split into words */
    if (verbatim) {
        printf ("%s", buffer);
        newline ();
        return;
    }
    p = strchr (buffer, CHAR_LINK_POINTER);
    if (p) {
        char *q;
        
        *p = 0;
        print_string (buffer);
        q = strchr (p + 1, CHAR_LINK_END);
        if (q) {
            *q = 0;
            printf ("%c%s%c", CHAR_LINK_POINTER, p + 1, CHAR_LINK_END);
            print_string (q + 1);
        } else {
            /* Error, but try to handle it somehow */
            printf ("%c", CHAR_LINK_END);
        }
        return;
    }
    buffer = strtok (buffer, " \t\n");
    /* Repeat for each word */
    while (buffer){
	/* Skip empty strings */  
	if (strlen (buffer) > 0){
	    len = string_len (buffer);
	    /* Change the line if about to break the right margin */
	    if (col + len >= width)
		newline ();
	    /* Words are separated by spaces */
	    if (col > curindent){
		printf (" ");
		col ++;
            }
	    printf ("%s", buffer);
	    /* Increase column */
	    col += len;
	}
	/* Get the next word */
	buffer = strtok (NULL, " \t\n");
    } /* while */
    if (freshnl) {
        firstlen = col - curindent;
        freshnl = 0;
    }
}

/* Like print_string but with printf-like syntax */
void printf_string (char *format, ...)
{
    va_list args;
    char buffer [BUFFER_SIZE];

    va_start (args, format);
    vsprintf (buffer, format, args);
    va_end (args);
    print_string (buffer);
}

int main (int argc, char **argv)
{
    int len;			/* Length of input line */
    char buffer [BUFFER_SIZE];	/* Input line */
    int i, j;
    char *p; 
    int ignore_newline = 0;

    /* Validity check for arguments */
    if (argc != 3 || (width = atoi (argv[1])) <= 10){
	fprintf (stderr, _("Usage: fixhlp <width> <tocname>\n"));
	return 3;
    }
    
    if ((toc_file = fopen (argv[2], "w")) == NULL) {
    	fprintf (stderr, _("fixhlp: Cannot open toc for writing"));
    	return 4;
    }
    fprintf (toc_file, _("\04[Contents]\n Topics:\n\n"));

    /* Repeat for each input line */
    while (!feof (stdin)){
	/* Read a line */
	if (!fgets (buffer, BUFFER_SIZE, stdin)){
	    break;
	}
	in_row ++;
	len = strlen (buffer);
	/* Remove terminating newline */
	if (buffer [len-1] == '\n')
	{
	    len --;
	    buffer [len] = 0;
	}
	if (!buffer[0]) {
	    if (ignore_newline)
	        ignore_newline = 0;
	    else
	        newline ();
	} else {
	    if (buffer [0] == 4 && buffer [1] == '[') {
	        for (p = buffer + 2; *p == ' '; p++);
	        fprintf (toc_file, "%*s\01 %s \02%s\03\n", p - buffer + 1, "", p, p);
	        printf ("\04[%s]\n %s", p, p);
	    } else if (buffer [0] == CHAR_RESERVED && buffer [1] == '"') {
	        continue;
	    } else {
	        char *p, *q;
	        int i;
	        
	        for (p = buffer, q = strchr (p, CHAR_RESERVED); q != NULL;
	            p = q + 1, q = strchr (p, CHAR_RESERVED)) {
	            *q = 0;
	            if (*p)
	                print_string (p);
	            q++;
	            if (*q == '/')
	                ignore_newline = 1;
	            else if (*q == 'v')
	                verbatim = 1;
	            else if (*q == 'n')
	                verbatim = 0;
	            else {
	                indent = *q - '0';
	                if (ignore_newline) {
	                    i = firstlen;
	                    if (i > indent - curindent - 1)
	                        ignore_newline = 0;
	                    else {
	                        i = indent - curindent - i - 1;
	                        printf ("%*s", i, "");
	                        col += i;
	                    }
	                }
	            }
	        }
	    	print_string (p);
	    }
	}
    }

    /* All done */
    newline ();
    return 0;
}
