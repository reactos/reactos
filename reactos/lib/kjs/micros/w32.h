/*
 * W32 specific definitions.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/micros/w32.h,v $
 * $Id: w32.h,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#ifndef W32_H
#define W32_H

/* For chmod() & _find{first,next,close}(). */
#include <io.h>

/* For alloca(). */
#include <malloc.h>

/* For chdir(). */
#include <direct.h>

/*
 * Types and definitions.
 */

#define popen(cmd, mode) (NULL)
#define pclose(fp) (1)

struct dirent
{
  char *d_name;
};

struct DIR_st
{
  char *path;
  long handle;

  /* The position in the directory. */
  unsigned int pos;

  struct _finddata_t finddata;
  struct dirent de;
};

typedef struct DIR_st DIR;


/*
 * Prototypes for functions, defined in w32.c.
 */

extern unsigned int sleep (unsigned int seconds);
extern unsigned int usleep (unsigned int useconds);

/* Directory handling. */

DIR *opendir (const char *name);

struct dirent *readdir (DIR *dir);

int closedir (DIR *dir);

void rewinddir (DIR *dir);

void seekdir (DIR *dir, long offset);

long telldir (DIR *dir);

#endif /* not W32_H */
