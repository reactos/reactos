/*
*    SORT - reads line of a file and sorts them in order
*    Copyright  1995  Jim Lynch
*
*    Adapted for ReactOS
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define MAXRECORDS  65536	/* maximum number of records that can be
				 * sorted */
#define MAXLEN	4095		/* maximum record length */

int             rev;		/* reverse flag */
int             help;		/* help flag */
int             sortcol;	/* sort column */
int             err = 0;	/* error counter */

int
cmpr(void *a, void *b)
{
    char           *A, *B;

    A = *(char **) a;
    B = *(char **) b;

    if (sortcol > 0) {
	if (strlen(A) > sortcol)
	    A += sortcol;
	else
	    A = "";
	if (strlen(B) > sortcol)
	    B += sortcol;
	else
	    B = "";
    }
    if (!rev)
	return strcmp(A, B);
    else
	return strcmp(B, A);
}

void
usage(void)
{
    fputs("SORT\n", stderr);
    fputs("Sorts input and writes output to a file, console or a device.\n", stderr);
    if (err)
	fputs("Invalid parameter\n", stderr);
    fputs("    SORT [options] < [drive:1][path1]file1 > [drive2:][path2]file2\n", stderr);
    fputs("    Command | SORT [options] > [drive:][path]file\n", stderr);
    fputs("    Options:\n", stderr);
    fputs("    /R	Reverse order\n", stderr);
    fputs("    /+n	Start sorting with column n\n", stderr);
    fputs("    /?	Help\n", stderr);
}

int main(int argc, char **argv)
{
    char            temp[MAXLEN + 1];
    char          **list;
    char           *cp;		/* option character pointer */
    int             nr;
    int             i;


    sortcol = 0;
    rev = 0;
    while (--argc) {
	if (*(cp = *++argv) == '/') {
	    switch (cp[1]) {
	    case 'R':
	    case 'r':
		rev = 1;
		break;
	    case '?':
	    case 'h':
	    case 'H':
		help = 1;
		break;
	    case '+':
		sortcol = atoi(cp + 1);
		if (sortcol)
		    sortcol--;
		break;
	    default:
		err++;
	    }
	}
    }
    if (err || help) {
	usage();
	exit(1);
    }
    list = (char **) malloc(MAXRECORDS * sizeof(char *));
    if (list == NULL) {
        fputs("SORT: Insufficient memory\n", stderr);
        exit(3);
    }
    for (nr = 0; nr < MAXRECORDS; nr++) {
	if (fgets(temp, MAXLEN, stdin) == NULL)
	    break;
	if(strlen(temp))
	    temp[strlen(temp)-1]='\0';
	list[nr] = (char *) malloc(strlen(temp) + 1);
	if (list[nr] == NULL) {
	    fputs("SORT: Insufficient memory\n", stderr);
	    exit(3);
	}
	strcpy(list[nr], temp);
    }
    if (nr == MAXRECORDS) {
	fputs("SORT: number of records exceeds maximum\n", stderr);
	exit(4);
    }
    qsort((void *) list, nr, sizeof(char *), cmpr);
    for (i = 0; i < nr; i++) {
	fputs(list[i], stdout);
	fputs("\n",stdout);
    }
    return 0;
}

/* EOF */
