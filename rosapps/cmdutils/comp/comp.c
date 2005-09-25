/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS comp utility
 * FILE:        comp.c
 * PURPOSE:     Compares the contents of two files
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 25/09/05 Created
 *
 */


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRBUF 1024

/* getline:  read a line, return length */
INT GetLine(char *line, FILE *in)
{
    if (fgets(line, STRBUF, in) == NULL)
        return 0;
    else
        return strlen(line);
}

/* print program usage */
VOID Usage(VOID)
{
    _tprintf(_T("\nCompares the contents of two files or sets of files.\n\n"
                "COMP [data1] [data2]\n\n"
                "  data1      Specifies location and name of first file to compare.\n"
                "  data2      Specifies location and name of second file to compare.\n"));
}


int _tmain (int argc, TCHAR *argv[])
{
    INT i;
    INT LineLen1, LineLen2;
    FILE *fp1, *fp2;           // file pointers
    PTCHAR Line1 = (TCHAR *)malloc(STRBUF * sizeof(TCHAR));
    PTCHAR Line2 = (TCHAR *)malloc(STRBUF * sizeof(TCHAR));
    TCHAR File1[_MAX_PATH],    // file paths
          File2[_MAX_PATH];
    BOOL bMatch = TRUE,        // files match
         bAscii = FALSE,       // /A switch
         bLineNos = FALSE;     // /L switch

    /* parse command line for options */
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            --argc;
            switch (argv[i][1]) {
               case 'A': bAscii = TRUE;
                         _tprintf(_T("/a not Supported\n")); /*FIXME: needs adding */
                         break;
               case 'L': bLineNos = TRUE;
                         _tprintf(_T("/l not supported\n")); /*FIXME: needs adding */
                         break;
               case '?': Usage();
                         return EXIT_SUCCESS;
               default:
                   _tprintf(_T("Invalid switch - /%c\n"), argv[i][1]);
                   Usage();
                   return EXIT_FAILURE;
            }
        }
    }

    switch (argc)
    {
        case 1 :
                 _tprintf(_T("Name of first file to compare: "));
                 fgets(File1, _MAX_PATH, stdin);
                 for (i=0; i<_MAX_PATH; i++)
                 {
                     if (File1[i] == '\n')
                     {
                         File1[i] = '\0';
                         break;
                     }
                 }

                 _tprintf(_T("Name of second file to compare: "));
                 fgets(File2, _MAX_PATH, stdin);
                 for (i=0; i<_MAX_PATH; i++)
                 {
                     if (File2[i] == '\n')
                     {
                         File2[i] = '\0';
                         break;
                     }
                 }
                 break;
        case 2 :
                 _tcsncpy(File1, argv[1], _MAX_PATH);
                 _tprintf(_T("Name of second file to compare: "));
                 fgets(File2, _MAX_PATH, stdin);
                 for (i=0; i<_MAX_PATH; i++)
                 {
                     if (File2[i] == '\n')
                     {
                         File2[i] = '\0';
                         break;
                     }
                 }
                 break;
        case 3 :
                 _tcsncpy(File1, argv[1], _MAX_PATH);
                 _tcsncpy(File2, argv[2], _MAX_PATH);
                 break;
        default :
                  _tprintf(_T("Bad command line syntax\n"));
                  return EXIT_FAILURE;
                  break;
    }



    if ((fp1 = fopen(File1, "r")) == NULL)
    {
        _tprintf(_T("Can't find/open file: %s\n"), File1);
        return EXIT_FAILURE;
    }
    if ((fp2 = fopen(File2, "r")) == NULL)
    {
        _tprintf(_T("Can't find/open file: %s\n"), File2);
        return EXIT_FAILURE;
    }


    _tprintf(_T("Comparing %s and %s...\n"), File1, File2);

    while ((LineLen1 = GetLine(Line1, fp1) != 0) &&
           (LineLen2 = GetLine(Line2, fp2) != 0))
    {
        // LineCount++;
        while ((*Line1 != '\0') && (*Line2 != '\0'))
        {
            if (*Line1 != *Line2)
            {
                bMatch = FALSE;
                break;
            }
            Line1++, Line2++;
        }
    }

    bMatch ? _tprintf(_T("Files compare OK\n")) : _tprintf(_T("Files are different sizes.\n"));

    fclose(fp1);
    fclose(fp2);


    return EXIT_SUCCESS;
}
/* EOF */
