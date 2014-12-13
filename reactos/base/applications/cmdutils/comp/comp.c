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
#include <assert.h>

#define STRBUF 1024

/* getline:  read a line, return length */
INT GetBuff(char *buff, FILE *in)
{
    return fread(buff, 1, STRBUF, in);
}

INT FileSize(FILE * fd) {
    INT result = -1;
    if (fseek(fd, 0, SEEK_END) == 0 && (result = ftell(fd)) != -1)
    {
        //restoring file pointer
        rewind(fd);
    }
    return result;
}

/* print program usage */
VOID Usage(VOID)
{
    _tprintf(_T("\nCompares the contents of two files or sets of files.\n\n"
                "COMP [/L] [/A] [data1] [data2]\n\n"
                "  data1      Specifies location and name of first file to compare.\n"
                "  data2      Specifies location and name of second file to compare.\n"
                "  /A         Display differences in ASCII characters.\n"
                "  /L         Display line numbers for differences.\n"));
}


int _tmain (int argc, TCHAR *argv[])
{
    INT i;
    // file pointers
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    INT BufLen1, BufLen2;
    PTCHAR Buff1 = NULL;
    PTCHAR Buff2 = NULL;
    TCHAR File1[_MAX_PATH + 1],    // file paths
          File2[_MAX_PATH + 1];
    BOOL bAscii = FALSE,       // /A switch
         bLineNos = FALSE;     // /L switch
    UINT  LineNumber;
    UINT  Offset;
    INT  FileSizeFile1;
    INT  FileSizeFile2;
    INT  NumberOfOptions = 0;
    INT  FilesOK = 1;
    INT Status = EXIT_SUCCESS;

    /* parse command line for options */
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            switch (argv[i][1]) {
               case 'A': bAscii = TRUE;
                         NumberOfOptions++;
                         break;
               case 'L': bLineNos = TRUE;
                         NumberOfOptions++;
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

    if (argc - NumberOfOptions == 3)
    {
        _tcsncpy(File1, argv[1 + NumberOfOptions], _MAX_PATH);
        _tcsncpy(File2, argv[2 + NumberOfOptions], _MAX_PATH);
    } else {
        _tprintf(_T("Bad command line syntax\n"));
        return EXIT_FAILURE;
    }
    
    Buff1 = (TCHAR *)malloc(STRBUF * sizeof(TCHAR));
    if (Buff1 == NULL)
    {
        _tprintf(_T("Can't get free memory for Buff1\n"));
        return EXIT_FAILURE;
    }

    Buff2 = (TCHAR *)malloc(STRBUF * sizeof(TCHAR));
    if (Buff2 == NULL)
    {
        _tprintf(_T("Can't get free memory for Buff2\n"));
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    if ((fp1 = fopen(File1, "rb")) == NULL)
    {
        _tprintf(_T("Can't find/open file: %s\n"), File1);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }
    if ((fp2 = fopen(File2, "rb")) == NULL)
    {
        _tprintf(_T("Can't find/open file: %s\n"), File2);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }


    _tprintf(_T("Comparing %s and %s...\n"), File1, File2);

    FileSizeFile1 = FileSize(fp1);
    if (FileSizeFile1 == -1) 
    {
        _tprintf(_T("Can't determine size of file: %s\n"), File1);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    FileSizeFile2 = FileSize(fp2);
    if (FileSizeFile2 == -1) 
    {
        _tprintf(_T("Can't determine size of file: %s\n"), File2);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    if (FileSizeFile1 != FileSizeFile2)
    {
        _tprintf(_T("Files are different sizes.\n"));
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    LineNumber = 1;
    Offset = 0;
    while (1) 
    {
        BufLen1 = GetBuff(Buff1, fp1);
        BufLen2 = GetBuff(Buff2, fp2);

        if (ferror(fp1) || ferror(fp2)) 
        {
            _tprintf(_T("Files read error.\n"));
            Status = EXIT_FAILURE;
            goto Cleanup;
        }

        if (!BufLen1 && !BufLen2) 
            break;

        assert(BufLen1 == BufLen2);
        for (i = 0; i < BufLen1; i++) 
        {
            if (Buff1[i] != Buff2[i])
            {
                FilesOK = 0;

                //Reporting here a mismatch
                if (bLineNos) 
                {
                    _tprintf(_T("Compare error at LINE %d\n"), LineNumber);
                }
                else
                {
                    _tprintf(_T("Compare error at OFFSET %d\n"), Offset);
                }
                
                if (bAscii)
                {
                    _tprintf(_T("file1 = %c\n"), Buff1[i]);
                    _tprintf(_T("file2 = %c\n"), Buff2[i]);
                }
                else
                {
                    _tprintf(_T("file1 = %X\n"), Buff1[i]);
                    _tprintf(_T("file2 = %X\n"), Buff2[i]);
                }

                Offset++;

                if (Buff1[i] == '\n')
                    LineNumber++;
            }
         }
    }

    if (FilesOK)
        _tprintf(_T("Files compare OK\n"));
    
Cleanup:

    if(fp1)
        fclose(fp1);
    if(fp2)
        fclose(fp2);

    if(Buff1)
        free(Buff1);
    if(Buff2)
        free(Buff2);

    return Status;
}
/* EOF */
