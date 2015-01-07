/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/binhive.c
 * PURPOSE:         Binary hive export code
 * PROGRAMMER:      Hervé Poussineau
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>

#include "mkhive.h"

BOOL
ExportBinaryHive(
    IN PCSTR FileName,
    IN PCMHIVE Hive)
{
    FILE *File;
    BOOL ret;

    printf ("  Creating binary hive: %s\n", FileName);

    /* Create new hive file */
    File = fopen (FileName, "w+b");
    if (File == NULL)
    {
        printf("    Error creating/opening file\n");
        return FALSE;
    }

    fseek (File, 0, SEEK_SET);

    Hive->FileHandles[HFILE_TYPE_PRIMARY] = (HANDLE)File;
    ret = HvWriteHive(&Hive->Hive);
    fclose (File);
    return ret;
}

/* EOF */
