/*
 * W32 specific functions.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/micros/w32.c,v $
 * $Id: w32.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include <jsint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * Global functions.
 */

unsigned int
sleep (unsigned int seconds)
{
  Sleep ((ULONG) seconds * 1000);

  /* XXX Should count how many seconcs we actually slept and return it. */
  return 0;
}


int
usleep (unsigned int useconds)
{
  Sleep (useconds);
  return 0;
}


/*
 * Directory handling.
 */

DIR *
opendir (const char *name)
{
  DIR *dir;

  dir = calloc (1, sizeof (*dir));
  if (dir == NULL)
    return NULL;

  dir->path = malloc (strlen (name) + 5);
  if (dir->path == NULL)
    {
      free (dir);
      return NULL;
    }
  sprintf (dir->path, "%s\\*.*", name);

  dir->handle = _findfirst (dir->path, &dir->finddata);
  dir->pos = 0;

  return dir;
}


struct dirent *
readdir (DIR *dir)
{
  switch (dir->pos)
    {
    case 0:
      dir->de.d_name = ".";
      break;

    case 1:
      dir->de.d_name = "..";
      break;

    case 2:
      if (dir->handle < 0)
	/* It was an empty directory. */
	return NULL;

      dir->de.d_name = dir->finddata.name;
      break;

    default:
      if (_findnext (dir->handle, &dir->finddata) < 0)
	return NULL;

      dir->de.d_name = dir->finddata.name;
      break;
    }

  dir->pos++;

  return &dir->de;
}


int
closedir (DIR *dir)
{
  _findclose (dir->handle);
  free (dir->path);
  free (dir);

  return 0;
}


void
rewinddir (DIR *dir)
{
  _findclose (dir->handle);

  dir->pos = 0;
  dir->handle = _findfirst (dir->path, &dir->finddata);
}


void
seekdir (DIR *dir, long offset)
{
}

long
telldir (DIR *dir)
{
  return -1;
}
