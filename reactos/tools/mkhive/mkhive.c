/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: mkhive.c,v 1.1 2003/04/14 17:18:48 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/mkhive.c
 * PURPOSE:         Hive maker
 * PROGRAMMER:      Eric Kohl
 */

#include <limits.h>
#include <string.h>

#include "mkhive.h"
#include "registry.h"
#include "reginf.h"
#include "binhive.h"

#ifndef WIN32
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif


void usage (void)
{
  printf ("Usage: mkhive <srcdir> <dstdir>\n\n");
}


int main (int argc, char *argv[])
{
  char FileName[PATH_MAX];

  if (argc < 3)
    {
      usage ();
    }

  RegInitializeRegistry ();

  strcpy (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivesys.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

#if 0
  strcpy (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivecls.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

  strcpy (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivesft.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

  strcpy (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivedef.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);
#endif

  strcpy (FileName, argv[2]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "system");
  ExportBinaryHive (FileName, "\\Registy\\Machine\\System");


//  RegShutdownRegistry ();

  return 0;
}

/* EOF */
