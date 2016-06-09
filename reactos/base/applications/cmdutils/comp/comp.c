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
 * FILE:        base/applications/cmdutils/comp/comp.c
 * PURPOSE:     Compares the contents of two files
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "resource.h"

#define STRBUF 1024

VOID ResPrint(INT res_no, ...)
{
    TCHAR * res_string;
    va_list vargs;

    va_start(vargs, res_no);

    if (LoadString(GetModuleHandle(NULL), res_no, (TCHAR*)&res_string, 0))
    {
        _vtprintf(res_string, vargs);
    }
    else
    {
        _tprintf(_T("Resource loading error!"));
    }

    va_end(vargs);
}

/* getline:  read a line, return length */
INT GetBuff(PBYTE buff, FILE *in)
{
    return fread(buff, sizeof(BYTE), STRBUF, in);
}

INT FileSize(FILE * fd)
{
    INT result = -1;
    if (fseek(fd, 0, SEEK_END) == 0 && (result = ftell(fd)) != -1)
    {
        /* Restoring file pointer */
        rewind(fd);
    }
    return result;
}

/* Print program usage */
VOID Usage(VOID)
{
    ResPrint(IDS_HELP);
}


int _tmain (int argc, TCHAR *argv[])
{
    INT i;

    /* File pointers */
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;

    INT BufLen1, BufLen2;
    PBYTE Buff1 = NULL;
    PBYTE Buff2 = NULL;
    TCHAR File1[_MAX_PATH + 1], // File paths
          File2[_MAX_PATH + 1];
    BOOL bAscii = FALSE,        // /A switch
         bLineNos = FALSE;      // /L switch
    UINT  LineNumber;
    UINT  Offset;
    INT  FileSizeFile1;
    INT  FileSizeFile2;
    INT  NumberOfOptions = 0;
    INT  FilesOK = 1;
    INT Status = EXIT_SUCCESS;

    /* Parse command line for options */
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == _T('/'))
        {
            switch (argv[i][1])
            {
                case _T('A'):
                    bAscii = TRUE;
                    NumberOfOptions++;
                    break;

                case _T('L'):
                    bLineNos = TRUE;
                    NumberOfOptions++;
                    break;

                case _T('?'):
                    Usage();
                    return EXIT_SUCCESS;

                default:
                    ResPrint(IDS_INVALIDSWITCH, argv[i][1]);
                    Usage();
                    return EXIT_FAILURE;
            }
        }
    }

    if (argc - NumberOfOptions == 3)
    {
        _tcsncpy(File1, argv[1 + NumberOfOptions], _MAX_PATH);
        _tcsncpy(File2, argv[2 + NumberOfOptions], _MAX_PATH);
    }
    else
    {
        ResPrint(IDS_BADSYNTAX);
        return EXIT_FAILURE;
    }

    Buff1 = (PBYTE)malloc(STRBUF);
    if (Buff1 == NULL)
    {
        _tprintf(_T("Can't get free memory for Buff1\n"));
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    Buff2 = (PBYTE)malloc(STRBUF);
    if (Buff2 == NULL)
    {
        _tprintf(_T("Can't get free memory for Buff2\n"));
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    if ((fp1 = _tfopen(File1, _T("rb"))) == NULL)
    {
        ResPrint(IDS_FILEERROR, File1);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }
    if ((fp2 = _tfopen(File2, _T("rb"))) == NULL)
    {
        ResPrint(IDS_FILEERROR, File2);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }


    ResPrint(IDS_COMPARING, File1, File2);

    FileSizeFile1 = FileSize(fp1);
    if (FileSizeFile1 == -1)
    {
        ResPrint(IDS_FILESIZEERROR, File1);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    FileSizeFile2 = FileSize(fp2);
    if (FileSizeFile2 == -1)
    {
        ResPrint(IDS_FILESIZEERROR, File2);
        Status = EXIT_FAILURE;
        goto Cleanup;
    }

    if (FileSizeFile1 != FileSizeFile2)
    {
        ResPrint(IDS_SIZEDIFFERS);
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
            ResPrint(IDS_READERROR);
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

                /* Reporting here a mismatch */
                if (bLineNos)
                {
                    ResPrint(IDS_MISMATCHLINE, LineNumber);
                }
                else
                {
                    ResPrint(IDS_MISMATCHOFFSET, Offset);
                }

                if (bAscii)
                {
                    ResPrint(IDS_ASCIIDIFF, 1, Buff1[i]);
                    ResPrint(IDS_ASCIIDIFF, 2, Buff2[i]);
                }
                else
                {
                    ResPrint(IDS_HEXADECIMALDIFF, 1, Buff1[i]);
                    ResPrint(IDS_HEXADECIMALDIFF, 2, Buff2[i]);
                }

                Offset++;

                if (Buff1[i] == '\n')
                    LineNumber++;
            }
         }
    }

    if (FilesOK)
        ResPrint(IDS_MATCH);

Cleanup:
    if (fp2)
        fclose(fp2);
    if (fp1)
        fclose(fp1);

    if (Buff2)
        free(Buff2);
    if (Buff1)
        free(Buff1);

    return Status;
}

/* EOF */
