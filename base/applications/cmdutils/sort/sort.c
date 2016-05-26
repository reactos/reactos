/*
 * PROJECT:   SORT - Adapted for ReactOS
 * LICENSE:   GPL - See COPYING in the top level directory
 * PURPOSE:   Reads line of a file and sorts them in order
 * COPYRIGHT: Copyright 1995 Jim Lynch
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXRECORDS 65536 /* maximum number of records that can be sorted */
#define MAXLEN 4095 /* maximum record length */

/* Reverse flag */
int rev;

/* Help flag */
int help;

/* Sort column */
int sortcol;

/* Error counter */
int err = 0;

int cmpr(const void *a, const void *b)
{
    char *A, *B;

    A = *(char **) a;
    B = *(char **) b;

    if (sortcol > 0)
    {
        if (strlen(A) > sortcol)
        {
            A += sortcol;
        }
        else
        {
            A = "";
        }
        if (strlen(B) > sortcol)
        {
            B += sortcol;
        }
        else
        {
            B = "";
        }
    }

    if (!rev)
    {
        return strcmp(A, B);
    }
    else
    {
        return strcmp(B, A);
    }
}

void usage(void)
{
    fputs("SORT\n", stderr);
    fputs("Sorts input and writes output to a file, console or a device.\n",
          stderr);

    if (err)
    {
        fputs("Invalid parameter\n", stderr);
    }

    fputs("    SORT [options] < [drive:1][path1]file1 > [drive2:][path2]file2\n",
          stderr);

    fputs("    Command | SORT [options] > [drive:][path]file\n", stderr);
    fputs("    Options:\n", stderr);
    fputs("    /R   Reverse order\n", stderr);
    fputs("    /+n  Start sorting with column n\n", stderr);
    fputs("    /?   Help\n", stderr);
}

int main(int argc, char **argv)
{
    char temp[MAXLEN + 1];
    char **list;

    /* Option character pointer */
    char *cp;
    int i, nr;

    sortcol = 0;
    rev = 0;
    while (--argc)
    {
        if (*(cp = *++argv) == '/')
        {
            switch (cp[1])
            {
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
                    {
                        sortcol--;
                    }
                    break;

                default:
                    err++;
            }
        }
    }

    if (err || help)
    {
        usage();
        exit(1);
    }

    list = (char **) malloc(MAXRECORDS * sizeof(char *));
    if (list == NULL)
    {
        fputs("SORT: Insufficient memory\n", stderr);
        exit(3);
    }

    for (nr = 0; nr < MAXRECORDS; nr++)
    {
        if (fgets(temp, MAXLEN, stdin) == NULL)
        {
            break;
        }

        if(strlen(temp))
        {
            temp[strlen(temp) - 1] = '\0';
        }

        list[nr] = (char *) malloc(strlen(temp) + 1);
        if (list[nr] == NULL)
        {
            fputs("SORT: Insufficient memory\n", stderr);

            /* Cleanup memory */
            for (i = 0; i < nr; i++)
            {
                free(list[i]);
            }
            free(list);
            exit(3);
        }

        strcpy(list[nr], temp);
    }

    if (nr == MAXRECORDS)
    {
        fputs("SORT: number of records exceeds maximum\n", stderr);

        /* Cleanup memory */
        for (i = 0; i < nr; i++)
        {
            free(list[i]);
        }
        free(list);

        /* Bail out */
        exit(4);
    }

    qsort((void *)list, nr, sizeof(char *), cmpr);

    for (i = 0; i < nr; i++)
    {
        fputs(list[i], stdout);
        fputs("\n", stdout);
    }

    /* Cleanup memory */
    for (i = 0; i < nr; i++)
    {
        free(list[i]);
    }
    free(list);
    return 0;
}
/* EOF */
